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

#include <glosm/util/gl.h>

#include <glosm/GPXDatasource.hh>
#include <glosm/HeightmapDatasource.hh>

#include <glosm/GPXTile.hh>

#include <glosm/Projection.hh>
#include <glosm/VertexBuffer.hh>

#include <iostream>
#include <glosm/ParsingHelpers.hh>
#include "3dsModel/ModelReader.h"
#include <json/json.h>
#include <filesystem>
#include <string>

std::vector<std::pair<Vector3i, Vector3f>> MakeVector()
{
	ladujModele();
	
	Json::Value root;
	Json::CharReaderBuilder builder;
	JSONCPP_STRING errs;
	std::vector<std::pair<Vector3i, Vector3f>> ship_vector;

	std::ifstream ifs;
	if (ifs) {
		ifs.open("../../testdata/positions.json");

		if (!Json::parseFromStream(builder, ifs, &root, &errs)) {
			std::cout << errs << std::endl;
		}

		for( Json::Value::const_iterator itr = root.begin() ; itr != root.end() ; itr++ ) {
			//std::cout << itr.key().asString() << std::endl;
			//std::cout << *itr << std::endl;
			//std::cout << root[itr.key()] << std::endl;

			std::string lat = root[itr.key().asString()]["Latitude"].asString();
			std::string lon = root[itr.key().asString()]["Longitude"].asString();

			std::string cog = root[itr.key().asString()]["Cog"].asString();
			std::string sog = root[itr.key().asString()]["Sog"].asString();

			//std::cout << lat.c_str() << "  " << lon.c_str() << std::endl;

			int latInt = ParseCoord(lat.c_str());
			int lonInt = ParseCoord(lon.c_str());

			auto cogInt = atof(cog.c_str());
			auto sogInt = atof(sog.c_str());
			//auto sogInt = 1.0;

			//std::cout << cogInt << " " << sogInt << std::endl;

			auto ship = Vector3i(lonInt, latInt, 0);
			auto details = Vector3f(cogInt, sogInt, sogInt);
			ship_vector.push_back(std::make_pair(ship, details));
		}
	}
	else 
	{
		std::cout << "error. Positions file not found" << std::endl;
	}

	return ship_vector;
}

std::vector<std::pair<Vector3i, Vector3f>> GPXTile::shipsGlobal = MakeVector();

GPXTile::GPXTile(const Projection& projection, const GPXDatasource& datasource, const HeightmapDatasource& heightmap, const Vector2i& ref, const BBoxi& bbox) : Tile(ref), size_(0) {
	std::vector<Vector3i> points;
	datasource.GetPoints(points, bbox);

	if (!points.empty()) {
		points_.reset(new VertexBuffer<Vector3f>(GL_ARRAY_BUFFER));

		points_->Data().reserve(2 * points.size());
		for (std::vector<Vector3i>::const_iterator i = points.begin(); i != points.end(); ++i) {
			points_->Data().push_back(projection.Project(*i, ref));
			points_->Data().push_back(projection.Project(Vector3i(i->x, i->y, heightmap.GetHeight(*i)), ref));
		}

		size_ = points_->GetFootprint();
	}
	std::cout << "InitGPX" << std::endl;

	/********************************
	*								*
	*								*
	*          LOAD SHIPS			*
	*								*
	*								*
	********************************/

	
	for (std::vector<std::pair<Vector3i, Vector3f>>::const_iterator i = shipsGlobal.begin(); i != shipsGlobal.end(); ++i) {
		auto ship = Vector3i(i->first.x, i->first.y, 0);
		Vector3f shipFloat = projection.Project(ship, ref);
		ships.push_back(shipFloat);
	}

	//std::filesystem::path cwd = std::filesystem::current_path();
	//std::cout << cwd << std::endl;
}

GPXTile::~GPXTile() {
}

void GPXTile::Render() {

	if (points_.get()) {
		glDepthFunc(GL_LEQUAL);

		//glColor4f(1.0f, 0.0f, 1.0f, 0.5f);
		//glPointSize(3.0);

		points_->Bind();

		
		glEnableClientState(GL_VERTEX_ARRAY);

		std::cout << " \n\n\n\n" << std::endl;
		for ( int i=0; i<ships.size(); i++ )
		{
			glPushMatrix();
				glTranslated(ships[i].x, ships[i].y, -0.00015 );
				glRotated(shipsGlobal[i].second.x,0,0,1);
				glTranslated(0, 0.0001 * shipsGlobal[i].second.y, 0 );
				glRotated(180,1,0,0);
				glScaled(0.0000001, 0.0000001, 0.0000001);
				rysujModel ("scene");
			glPopMatrix();
		} 


		//glVertexPointer(3, GL_FLOAT, sizeof(Vector3f)*2, BUFFER_OFFSET(0));
		//glDrawArrays(GL_POINTS, 0, points_->GetSize()/2);

		//glVertexPointer(3, GL_FLOAT, sizeof(Vector3f), BUFFER_OFFSET(0));
		//glDrawArrays(GL_LINES, 0, points_->GetSize());

		//glPushMatrix();
		//	glTranslated(ship.x,ship.y,0);
		//	glRotated(90,1,0,0);
		//  glScaled(0.0000001, 0.0000001, 0.0000001);
		//	rysujModel ("scene");
		//glPopMatrix();
	
		glDisableClientState(GL_VERTEX_ARRAY);
	}
}

size_t GPXTile::GetSize() const {
	return size_;
}
