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

	char* lat =  "53.8931081";
	char* lon = "-29.5387218";
	int latInt = ParseCoord(lat);
	int lonInt = ParseCoord(lon);
	Vector3i globalVector = Vector3i(lonInt , latInt, 0);
	ship = projection.Project(globalVector, ref);
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

		glVertexPointer(3, GL_FLOAT, sizeof(Vector3f)*2, BUFFER_OFFSET(0));
		glDrawArrays(GL_POINTS, 0, points_->GetSize()/2);

		glVertexPointer(3, GL_FLOAT, sizeof(Vector3f), BUFFER_OFFSET(0));
		glDrawArrays(GL_LINES, 0, points_->GetSize());


		glBegin(GL_TRIANGLES);
			glVertex2f ( ship.x, ship.y );
			glVertex2f ( ship.x - 0.0001, ship.y);
			glVertex2f ( ship.x - 0.0002, ship.y - 0.0001);
		glEnd();


		glDisableClientState(GL_VERTEX_ARRAY);
	}
}

size_t GPXTile::GetSize() const {
	return size_;
}
