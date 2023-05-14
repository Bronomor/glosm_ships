/*
 * Copyright (C) 2010-2012 Dmitry Marakasov
 *
 * This file is part of glosm.
 *
 * glosm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * glosm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public
 * License along with glosm.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/*
 * Generic class for loading and storing OSM data
 *
 * Possible generic improvements:
 * - use hash maps instead of tree maps
 * - store tag names separately and operate with tag id
 *   - use (hash)map for forward lookup (tag -> id)
 *   - use vector for reverse lookup (id -> tag)
 * - use fastosm library
 * - use custom allocators for most data
 *
 * Space iprovements with complexity/speed cost
 * - prefix encoding for node coords and refs
 * - store nodes without tags in a separate or additional map
 * - store node coords directly in ways
 * Last two should save tons of memory and improve speed severely
 * by killing an indirection
 *
 * Improvements for non-generic use:
 * (tons)
 *
 * Other:
 * - may store relation id(s) for ways - at least for multipolygons
 */

#include <cstdlib>
#include <iostream>

#include <glosm/PreloadedXmlDatasource.hh>
#include <glosm/ParsingHelpers.hh>
#include <glosm/WayMerger.hh>

osmid_t PreloadedXmlDatasource::next_synthetic_id_ = std::numeric_limits<osmid_t>::max();

PreloadedXmlDatasource::PreloadedXmlDatasource() : XMLParser(XMLParser::HANDLE_ELEMENTS), bbox_(BBoxi::Empty()) {
}

PreloadedXmlDatasource::~PreloadedXmlDatasource() {
}

static void ParseTag(OsmDatasource::TagsMap& map, const char** atts) {
	std::string key;
	std::string value;
	for (const char** att = atts; *att; ++att) {
		if (StrEq<1>(*att, "k"))
			key = *(++att);
		else if (StrEq<1>(*att, "v"))
			value = *(++att);
		else
			++att;
	}

	map.insert(std::make_pair(key, value));
}

void PreloadedXmlDatasource::StartElement(const char* name, const char** atts) {
	if (tag_level_ == 1 && current_tag_ == OSM) {
		osmid_t id = 0;
		osmint_t lat = 0;
		osmint_t lon = 0;
		for (const char** att = atts; *att; ++att) {
			if (StrEq<1>(*att, "id"))
				id = strtoll(*(++att), NULL, 10);
			else if (StrEq<2>(*att, "lat"))
				lat = ParseCoord(*(++att));
			else if (StrEq<2>(*att, "lon"))
				lon = ParseCoord(*(++att));
			else
				++att;
		}

		if (StrEq<1>(name, "node")) {
			current_tag_ = NODE;
			std::pair<NodesMap::iterator, bool> p = nodes_.insert(std::make_pair(id, Node(lon, lat)));
			last_node_ = p.first;
			//last_node_tags_ = node_tags_.end();
		} else if (StrEq<1>(name, "way")) {
			current_tag_ = WAY;
			std::pair<WaysMap::iterator, bool> p = ways_.insert(std::make_pair(id, Way()));
			last_way_ = p.first;
		} else if (StrEq<1>(name, "relation")) {
			current_tag_ = RELATION;
			std::pair<RelationsMap::iterator, bool> p = relations_.insert(std::make_pair(id, Relation()));
			last_relation_ = p.first;
		} else if (StrEq<-1>(name, "bounds")) {
			bbox_.Include(ParseBounds(atts));
		} else if (StrEq<-1>(name, "bound")) {
			bbox_.Include(ParseBound(atts));
		}
	} else if (tag_level_ == 2 && current_tag_ == NODE) {
		if (last_node_ != nodes_.end()) {
			if (StrEq<0>(name, "tag")) {
//				if (last_node_tags_ == node_tags_.end()) {
//					std::pair<NodeTagsMap::iterator, bool> p = node_tags_.insert(std::make_pair(last_node_->first, TagsMap()));
//					last_node_tags_ = p.first;
//				}
//				ParseTag(last_node_tags_->second, atts);
			} else {
				throw ParsingException() << "unexpected tag in node";
			}
		}
	} else if (tag_level_ == 2 && current_tag_ == WAY) {
		if (last_way_ != ways_.end()) {
			if (StrEq<1>(name, "tag")) {
				ParseTag(last_way_->second.Tags, atts);
			} else if (StrEq<1>(name, "nd")) {
				osmid_t id;

				if (**atts && StrEq<0>(*atts, "ref"))
					id = strtoll(*(atts+1), NULL, 10);
				else
					throw ParsingException() << "no ref attribute for nd tag";

				last_way_->second.Nodes.push_back(id);
			} else {
				throw ParsingException() << "unexpected tag in way";
			}
		}
	} else if (tag_level_ == 2 && current_tag_ == RELATION) {
		if (last_relation_ != relations_.end()) {
			if (StrEq<1>(name, "tag")) {
				ParseTag(last_relation_->second.Tags, atts);
			} else if (StrEq<1>(name, "member")) {
				osmid_t ref = 0;
				const char* role = 0;
				Relation::Member::Type_t type = Relation::Member::UNKNOWN;

				for (const char** att = atts; *att; ++att) {
					if (StrEq<2>(*att, "ref"))
						ref = strtoll(*(++att), NULL, 10);
					else if (StrEq<1>(*att, "type")) {
						++att;
						if (StrEq<1>(*att, "node"))
							type = Relation::Member::NODE;
						else if (StrEq<1>(*att, "way"))
							type = Relation::Member::WAY;
						else if (StrEq<1>(*att, "relation"))
							type = Relation::Member::RELATION;
						else
							throw ParsingException() << "bad relation member role";
					} else if (StrEq<2>(*att, "role")) {
						role = *(++att);
					} else {
						throw ParsingException() << "unexpected attribute in relation member";
					}
				}

				if (ref == 0 || role == NULL || type == Relation::Member::UNKNOWN)
					throw ParsingException() << "bad relation member";

				last_relation_->second.Members.push_back(Relation::Member(type, ref, role));
			} else {
				throw ParsingException() << "unexpected tag in relation";
			}
		}
	} else if (tag_level_ == 0 && current_tag_ == NONE && StrEq<-1>(name, "osm")) {
		current_tag_ = OSM;
	} else if (tag_level_ == 0) {
		throw ParsingException() << "unexpected root element (" << name << " instead of osm)";
	}

	++tag_level_;
}

void PreloadedXmlDatasource::EndElement(const char* /*name*/) {
	if (tag_level_ == 2) {
		switch (current_tag_) {
		case NODE:
			last_node_ = nodes_.end();
			current_tag_ = OSM;
			break;
		case WAY:
			FinalizeWay();
			last_way_ = ways_.end();
			current_tag_ = OSM;
			break;
		case RELATION:
			FinalizeRelation();
			last_relation_ = relations_.end();
			current_tag_ = OSM;
			break;
		default:
			break;
		}
	} else if (tag_level_ == 1 && current_tag_ == OSM) {
		current_tag_ = NONE;
	}

	--tag_level_;
}

void PreloadedXmlDatasource::FinalizeWay() {
	if (last_way_ == ways_.end())
		return;

	if (last_way_->second.Nodes.size() < 2) {
		std::cerr << "WARNING: way " << last_way_->first << " has < 2 nodes, dropping" << std::endl;
		ways_.erase_last();
		last_way_ = ways_.end();
		return;
	}

	/* check if a way is closed */
	if (last_way_->second.Nodes.front() == last_way_->second.Nodes.back()) {
		last_way_->second.Closed = true;

		/* check if a way is clockwise */
		NodesMap::const_iterator prev, cur;
		osmlong_t area = 0;
		for (Way::NodesList::const_iterator i = last_way_->second.Nodes.begin(); i != last_way_->second.Nodes.end(); ++i) {
			cur = nodes_.find(*i);
			if (cur == nodes_.end()) {
				//std::cerr << "WARNING: node " << *i << " referenced by way " << last_way_->first << " was not found in this dump, dropping way" << std::endl;
				ways_.erase_last();
				last_way_ = ways_.end();
				return;
			}
			if (i != last_way_->second.Nodes.begin())
				area += (osmlong_t)prev->second.Pos.x * cur->second.Pos.y - (osmlong_t)cur->second.Pos.x * prev->second.Pos.y;
			prev = cur;
			last_way_->second.BBox.Include(cur->second.Pos);
		}

		last_way_->second.Clockwise = area < 0;
	} else {
		for (Way::NodesList::const_iterator i = last_way_->second.Nodes.begin(); i != last_way_->second.Nodes.end(); ++i) {
			NodesMap::const_iterator cur = nodes_.find(*i);
			if (cur == nodes_.end()) {
				//std::cerr << "WARNING: node " << *i << " referenced by way " << last_way_->first << " was not found in this dump, dropping way" << std::endl;
				ways_.erase_last();
				last_way_ = ways_.end();
				return;
			}
			last_way_->second.BBox.Include(cur->second.Pos);
		}
	}
}

void PreloadedXmlDatasource::FinalizeRelation() {
	if (last_relation_ == relations_.end())
		return;

	TagsMap::const_iterator type = last_relation_->second.Tags.find("type");
	if (type == last_relation_->second.Tags.end() || type->second != "multipolygon")
		return;

	WayMerger merger;

	/* first, fill the merger with "outer" parts */
	for (Relation::MemberList::const_iterator member = last_relation_->second.Members.begin(); member != last_relation_->second.Members.end(); ++member) {
		if (member->Type != Relation::Member::WAY || member->Role != "outer")
			continue;

		WaysMap::const_iterator way = ways_.find(member->Ref);
		if (way == ways_.end()) {
			//std::cerr << "WARNING: way " << member->Ref << " referenced by relation " << last_relation_->first << " was not found in this dump, ignoring it" << std::endl;
			continue;
		}

		merger.AddWay(way->second.Nodes);
	}

	/* next, extract all complete merged ways */
	OsmDatasource::Way::NodesList tempnodes;
	while (merger.GetNextWay(tempnodes)) {
		std::pair<WaysMap::iterator, bool> p = ways_.insert(std::make_pair(next_synthetic_id_, Way()));
		assert(p.second);

		p.first->second.Nodes.swap(tempnodes);
		p.first->second.Tags = last_relation_->second.Tags;

		last_way_ = p.first;

		FinalizeWay();

		--next_synthetic_id_;
	}
}

Vector2i PreloadedXmlDatasource::GetCenter() const {
	return bbox_.GetCenter();
}

BBoxi PreloadedXmlDatasource::GetBBox() const {
	return bbox_;
}

void PreloadedXmlDatasource::Load(const char* filename) {
	bbox_ = BBoxi::Empty();
	current_tag_ = NONE;
	tag_level_ = 0;

	XMLParser::Load(filename);

	/* if file lacked bounding box, generate one ourselves */
	if (bbox_.IsEmpty()) {
		for (NodesMap::iterator node = nodes_.begin(); node != nodes_.end(); ++node)
			bbox_.Include(node->second.Pos);
	}
}

void PreloadedXmlDatasource::Clear() {
	nodes_.clear();
	ways_.clear();
	relations_.clear();
}

const OsmDatasource::Node& PreloadedXmlDatasource::GetNode(osmid_t id) const {
	NodesMap::const_iterator i = nodes_.find(id);
	if (i == nodes_.end())
		throw DataException() << "node not found";
	return i->second;
}

const OsmDatasource::Way& PreloadedXmlDatasource::GetWay(osmid_t id) const {
	WaysMap::const_iterator i = ways_.find(id);
	if (i == ways_.end())
		throw DataException() << "way not found";
	return i->second;
}

const OsmDatasource::Relation& PreloadedXmlDatasource::GetRelation(osmid_t id) const {
	RelationsMap::const_iterator i = relations_.find(id);
	if (i == relations_.end())
		throw DataException() << "relation not found";
	return i->second;
}

void PreloadedXmlDatasource::GetWays(std::vector<OsmDatasource::Way>& out, const BBoxi& bbox) const {
	if (!bbox.Intersects(bbox_))
		return;

	for (WaysMap::const_iterator i = ways_.begin(); i != ways_.end(); ++i)
		if (i->second.BBox.Intersects(bbox))
			out.push_back(i->second);
}
