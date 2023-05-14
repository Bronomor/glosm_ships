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

#ifndef GLOSMVIEWER_HH
#define GLOSMVIEWER_HH

#include <glosm/DummyHeightmap.hh>
#include <glosm/FirstPersonViewer.hh>
#include <glosm/GPXLayer.hh>
#include <glosm/GPXTile.hh>
#include <glosm/GeometryGenerator.hh>
#include <glosm/GeometryLayer.hh>
#include <glosm/PreloadedGPXDatasource.hh>
#include <glosm/PreloadedXmlDatasource.hh>
#include <glosm/Projection.hh>
#include <glosm/SRTMDatasource.hh>

#include <memory>

#include <sys/time.h>

#include <cpr/cpr.h>
#include <iostream>
#include <curl/curl.h>

class GlosmViewer {
public:
	enum Keys {
		KEY_UP = 0x100,
		KEY_DOWN,
		KEY_LEFT,
		KEY_RIGHT,
		KEY_SHIFT,
		KEY_CTRL
	};

	enum Buttons {
		BUTTON_LEFT,
		BUTTON_MIDDLE,
		BUTTON_RIGHT
	};

	Projection projection_;
	std::auto_ptr<GeometryGenerator> geometry_generator_;

protected:
	/* flags */
	bool no_glew_check_;

	double start_lon_;
	double start_lat_;
	double start_ele_;
	double start_yaw_;
	double start_pitch_;

	/* glosm objects */
	std::auto_ptr<FirstPersonViewer> viewer_;
	std::auto_ptr<PreloadedXmlDatasource> osm_datasource_;
	std::auto_ptr<PreloadedGPXDatasource> gpx_datasource_;
	std::auto_ptr<HeightmapDatasource> heightmap_datasource_;

	std::auto_ptr<GeometryLayer> ground_layer_;
	std::auto_ptr<GPXLayer> gpx_layer_;

	bool ground_shown_;
	bool gpx_shown_;

	int screenw_;
	int screenh_;

	struct timeval prevtime_, curtime_, fpstime_;
	int nframes_;

	int movementflags_;
	float speed_;
	bool slow_;
	bool fast_;
	int lockheight_;

	bool mouse_capture_;

	bool drag_;
	Vector2<int> drag_start_pos_;
	float drag_start_pitch_;
	float drag_start_yaw_;

protected:
	virtual void WarpCursor(int x, int y) = 0;
	virtual void Flip() = 0;
	virtual void ShowCursor(bool show) = 0;

public:
	GlosmViewer();

	virtual void Usage(int status, bool detailed, const char* progname);
	virtual void Init(int argc, char** argv);
	virtual void InitGL();
	virtual void Render();
	virtual void Resize(int w, int h);
	virtual void KeyDown(int key);
	virtual void KeyUp(int key);
	virtual void MouseMove(int x, int y);
	virtual void MouseButton(int button, bool pressed, int x, int y);
	void UpdateShips(){
		for ( int i=0; i<GPXTile::shipsGlobal.size(); i++ )
			GPXTile::shipsGlobal[i].second.y = GPXTile::shipsGlobal[i].second.y + GPXTile::shipsGlobal[i].second.z;
	};
};

#endif
