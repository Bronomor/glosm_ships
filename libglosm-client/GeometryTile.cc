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

#include <glosm/GeometryTile.hh>

#include <glosm/Projection.hh>
#include <glosm/Geometry.hh>
#include <glosm/VertexBuffer.hh>
#include <iostream>

GeometryTile::GeometryTile(const Projection& projection, const Geometry& geometry, const Vector2i& ref, const BBoxi& bbox) : Tile(ref), size_(0) {
	if (!geometry.GetLinesLengths().empty()) {
		lines_vertices_.reset(new VertexBuffer<Vector3f>(GL_ARRAY_BUFFER));
		lines_indices_.reset(new VertexBuffer<GLuint>(GL_ELEMENT_ARRAY_BUFFER));

		const Geometry::VertexVector& vertices = geometry.GetLinesVertices();
		const Geometry::LengthVector& lengths = geometry.GetLinesLengths();

		lines_vertices_->Data().reserve(vertices.size());

		for (unsigned int i = 0, curpos = 0; i < lengths.size(); ++i) {
#if defined(WITH_GLES)
			/* GL ES doesn't support VBOs larger than 65536 elements */
			/* @todo split into multiple VBOs */
			if (curpos + lengths[i] > 65536)
				break;
#endif

			for (int j = 0; j < lengths[i]; ++j) {
				lines_vertices_->Data().push_back(projection.Project(vertices[curpos + j], ref));
			}

			for (int j = 1; j < lengths[i]; ++j) {
				lines_indices_->Data().push_back(curpos + j - 1);
				lines_indices_->Data().push_back(curpos + j);
			}

			curpos += lengths[i];
		}

		size_ += lines_vertices_->GetFootprint() + lines_indices_->GetFootprint();
	}

	if (!geometry.GetConvexLengths().empty()) {
		convex_vertices_.reset(new VertexBuffer<Vertex>(GL_ARRAY_BUFFER));
		convex_indices_.reset(new VertexBuffer<GLuint>(GL_ELEMENT_ARRAY_BUFFER));

		const Geometry::VertexVector& vertices = geometry.GetConvexVertices();
		const Geometry::LengthVector& lengths = geometry.GetConvexLengths();

		convex_vertices_->Data().reserve(vertices.size());

		for (unsigned int i = 0, curpos = 0; i < lengths.size(); ++i) {
#if defined(WITH_GLES)
			/* GL ES doesn't support VBOs larger than 65536 elements */
			/* @todo split into multiple VBOs */
			if (curpos + lengths[i] > 65536)
				break;
#endif

			for (int j = 0; j < lengths[i]; ++j) {
				convex_vertices_->Data().push_back(Vertex(projection.Project(vertices[curpos + j], ref)));
			}

			for (int j = 2; j < lengths[i]; ++j) {
				convex_indices_->Data().push_back(curpos);
				convex_indices_->Data().push_back(curpos + j - 1);
				convex_indices_->Data().push_back(curpos + j);
			}

			CalcFanNormal(&convex_vertices_->Data()[curpos], lengths[i]);

			curpos += lengths[i];
		}

		size_ += convex_vertices_->GetFootprint() + convex_indices_->GetFootprint();
	}
}

GeometryTile::~GeometryTile() {
}

void GeometryTile::CalcFanNormal(Vertex* vertices, int count) {
	Vector3f first = vertices[1].pos - vertices[0].pos;
	Vector3f normal;
	for (int i = 1; i < count; ++i) {
		Vector3f v = vertices[i].pos - vertices[0].pos;

		if ((normal = first.CrossProduct(v)).LengthSquare() > 0)
			break;
	}
	normal.Normalize();

	for (int i = 0; i < count; ++i)
		vertices[i].norm = normal;
}

void GeometryTile::Render() {
	if (lines_vertices_.get()) {

		lines_vertices_->Bind();
		glEnableClientState(GL_VERTEX_ARRAY);
		//glColorPointer( 3, GL_UNSIGNED_BYTE, 0, colors );
		glVertexPointer(3, GL_FLOAT, sizeof(Vector3f), BUFFER_OFFSET(0));

		//glColor4f(0.6, 0.5, 0, 1);	
		glColor4f(0.1, 0.2, 0, 1);	
		//0.6/0.3/0.2
//		lines_indices_->Bind();
//		glDrawElements(GL_LINES, lines_indices_->GetSize(), GL_UNSIGNED_INT, 0);
		//glScaled(2,2,2);
		glDrawElements(GL_LINES, lines_indices_->GetSize(), GL_UNSIGNED_INT, lines_indices_->Data().data());
		//glScaled(0.1,0.1,0.1);
		lines_indices_->UnBind();
		//glColor4i(0, 0.6, 0, 0);

		glDisableClientState(GL_VERTEX_ARRAY);
	}
}

size_t GeometryTile::GetSize() const {
	return size_;
}
