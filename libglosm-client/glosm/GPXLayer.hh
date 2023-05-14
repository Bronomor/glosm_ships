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

#ifndef GPXLAYER_HH
#define GPXLAYER_HH

#include <glosm/Layer.hh>
#include <glosm/Projection.hh>
#include <glosm/NonCopyable.hh>
#include <glosm/TileManager.hh>

class Viewer;
class GPXDatasource;
class HeightmapDatasource;

/**
 * Layer with 3D OpenStreetMap data.
 */
class GPXLayer : public Layer, public TileManager, private NonCopyable {
public:
	enum Mode {
		NORMAL = 0, /* use real track elevations */
		FLAT = 1, /* place points on ground, don't use track elevations */
		SUBSTRACT = 2, /* substract heightmap from track elevation */
	};

protected:
	const Projection projection_;
	const GPXDatasource& datasource_;
	const HeightmapDatasource& heightmap_;

public:
	GPXLayer(const Projection projection, const GPXDatasource& datasource, const HeightmapDatasource& heightmap);
	virtual ~GPXLayer();

	void Render(const Viewer& viewer);
	virtual Tile* SpawnTile(const BBoxi& bbox, int flags) const;
	//void UpdateShips();
};

#endif
