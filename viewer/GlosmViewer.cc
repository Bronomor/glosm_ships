#define PY_SSIZE_T_CLEAN
#include <Python.h>
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

#include "GlosmViewer.hh"

#include <glosm/Math.hh>
#include <glosm/MercatorProjection.hh>
#include <glosm/SphericalProjection.hh>
#include <glosm/Timer.hh>
#include <glosm/CheckGL.hh>
#include <glosm/geomath.h>

#include <glosm/util/gl.h>

#include <getopt.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>

#include <cmath>
#include <cstdio>
#include <cstring>

#include <fstream>
#include <json/json.h>
#include <iostream>
#include <cmath>
#include <filesystem>

using recursive_directory_iterator = std::filesystem::recursive_directory_iterator;

void get_cpr_file(double longitude, double latitude, double progress, int64_t iterActual)
{
	std::string url = "https://api.openstreetmap.org/api/0.6/map?bbox="
	+std::to_string(longitude)
	+","
	+std::to_string(latitude)
	+","
	+std::to_string(longitude+progress)
	+","
	+std::to_string(latitude+progress);

	cpr::Response r = cpr::Get(cpr::Url{url});

	std::cout << r.status_code << std::endl;
	std::cout << r.header["content-type"] << std::endl;
	std::cout << url << std::endl;

	std::ofstream out("../../testdata/program/output"+std::to_string(iterActual)+".osm");
	out << r.text;
	out.close();
}

void gpx_cpr_load()
{
	// assumption square box
	double latitude = 64.52;
	double longitude = 11.2;
	double progress = 0.5;
	int index = 0;

	std::string url = "https://api.openstreetmap.org/api/0.6/trackpoints?bbox="
	+std::to_string(longitude)
	+","
	+std::to_string(latitude)
	+","
	+std::to_string(longitude+progress)
	+","
	+std::to_string(latitude+progress);

	cpr::Response r = cpr::Get(cpr::Url{url});
	std::cout << r.status_code << std::endl;
	std::cout << r.header["content-type"] << std::endl;
	std::cout << url << std::endl;

	std::ofstream out("../../testdata/tracks.gpx");
	out << r.text;
	out.close();
}

void cpr_load()
{
	// assumption square box
	double latitude_left = 64.52;
	double longitude_left = 11.2;
	double latitude_right = 65;
	double longitude_right = 11.68;

	double progress = 0.08;
	int64_t iterTotal = std::abs(latitude_right - latitude_left) / progress;
	int64_t iterActual = 0;

	for(double longitude=longitude_left; longitude<longitude_right; longitude+=progress){
		for(double latitude=latitude_left; latitude<latitude_right; latitude+=progress){
			get_cpr_file(longitude, latitude, progress, iterActual);
			iterActual += 1;
			//std::cout << iterActual << std::endl;
		}
	}

	//while(iterActual < iterTotal)
	//{
	//	double longitude = longitude_left + progress*iterActual;
	//	double latitude  = latitude_left  + progress*iterActual;
	//	get_cpr_file(longitude, latitude, progress);
	//	iterActual += 1;
	//}

	// cpr
	// data -> [[[64, 11], [64.5, 11.5]]]
	// cpr::Response r = cpr::Get(cpr::Url{"https://api.openstreetmap.org/api/0.6/map?bbox=14.3,54.1,14.425,54.225"});
	// /api/0.6/map?bbox=longitude_left,latitude_left,longitude_right,latitude_right
    // std::cout << r.status_code << std::endl;
    // r.text;
	// std::cout << r.text << std::endl; 
	// cpr
}


GlosmViewer::GlosmViewer() : projection_(MercatorProjection()), viewer_(new FirstPersonViewer) {
	screenw_ = screenh_ = 1;
	nframes_ = 0;

	movementflags_ = 0;
	speed_ = 200.0f;
	lockheight_ = 0;

	drag_ = slow_ = fast_ = false;

#if defined(WITH_TOUCHPAD)
	mouse_capture_ = false;
#else
	mouse_capture_ = true;
#endif

	/*************************
	*						 *
	*   DISABLE NOT USEFULL  *
	*		  LAYERS		 *
	*					     *
	*************************/

	ground_shown_ = true;
	gpx_shown_ = true;

	no_glew_check_ = false;

	start_lon_ = start_lat_ = start_ele_ = start_yaw_ = start_pitch_ = nan("");
}

void GlosmViewer::Usage(int status, bool detailed, const char* progname) {
	fprintf(stderr, "Usage: %s [-sfh] [-t <path>] [-l lon,lat,ele,yaw,pitch] <file.osm|-> [file.gpx ...]\n", progname);
	if (detailed) {
		fprintf(stderr, "Options:\n");
		//               [==================================72==================================]
		fprintf(stderr, "  -h       - show this help\n");
		fprintf(stderr, "  -s       - use spherical projection instead of mercator\n");
		fprintf(stderr, "  -t path  - add terrain layer, argument specifies path to directory\n");
		fprintf(stderr, "             with SRTM data (*.hgt files)\n");
		fprintf(stderr, "  -l ...   - set initial viewer's location and direction\n");
		fprintf(stderr, "             argument is comma-separated list of longitude, latitude,\n");
		fprintf(stderr, "             elevation, pitch and yaw, each of those may be empty for\n");
		fprintf(stderr, "             program default (e.g. -l ,,100.0,, or -l ,,100.0). Units\n");
		fprintf(stderr, "             are degrees and meters\n");
#if defined(WITH_GLEW)
		fprintf(stderr, "  -f       - ignore glew errors\n");
#endif
	}
	exit(status);
}

void GlosmViewer::Init(int argc, char** argv) {
	/* argument parsing */
	int c;
	const char* progname = argv[0];
	const char* srtmpath = NULL;
	while ((c = getopt(argc, argv, "sfht:l:")) != -1) {
		switch (c) {
		case 's': projection_ = SphericalProjection(); break;
		case 't': srtmpath = optarg; break;
		case 'l': {
					  int n = 0;
					  char* start = optarg;
					  char* end;
					  char* endptr;
					  do {
						  if ((end = strchr(start, ',')) != NULL)
							  *end = '\0';

						  double val = strtod(start, &endptr);

						  if (endptr != start) {
							  switch (n) {
							  case 0: start_lon_ = val; break;
							  case 1: start_lat_ = val; break;
							  case 2: start_ele_ = val; break;
							  case 3: start_yaw_ = val; break;
							  case 4: start_pitch_ = val; break;
							  }
						  }

						  n++;
						  start = end + 1;
					  } while(end != NULL);
				  } break;
#if defined(WITH_GLEW)
		case 'f': no_glew_check_ = true; break;
#endif
		case 'h': Usage(0, true, progname); break;
		default:
			Usage(1, false, progname);
		}
	}

	argc -= optind;
	argv += optind;

	/* load data */
	for (int narg = 0; narg < argc; ++narg) {
		std::string file = argv[narg];

		if (file == "-" || file.rfind(".osm") == file.length() - 4) {
			fprintf(stderr, "Loading %s as OSM...\n", file == "-" ? "stdin" : argv[narg]);
			if (osm_datasource_.get() == NULL) {
				Timer t;
				osm_datasource_.reset(new PreloadedXmlDatasource);

				/********************************
				*								*
				*								*
				*          LOAD OPEN			*
				*		  STREET MAP			*
				*								*
				********************************/

				//cpr_load();
				//gpx_cpr_load();
				const char* myPath = "../../testdata/program/";
				for(const auto& dirEntry : recursive_directory_iterator(myPath))
				{
					//std::cout << dirEntry << std::endl;
					std::string pathes = dirEntry.path().u8string();
					//	std::cout << pathes.c_str() << std::endl;
					osm_datasource_->Load(pathes.c_str());
				}

				//const char* box1 = "output1.osm";
				//const char* box2 = "../../testdata/program/output2.osm";
				//const char* box3 = "../../testdata/program/output3.osm";
				//const char* box4 = "../../testdata/program/output4.osm";
				
				//osm_datasource_->Load(box2);
				//osm_datasource_->Load(box3);
				//osm_datasource_->Load(box4);

				//osm_datasource_->Load(argv[narg]);
				// LOAD MORE OSM FILES


				fprintf(stderr, "Loaded in %.3f seconds\n", t.Count());
			} else {
				fprintf(stderr, "Only single OSM file may be loaded at once, skipped\n");
			}
		} else if (file.rfind(".gpx") == file.length() - 4) {
			fprintf(stderr, "Loading %s as GPX...\n", argv[narg]);
			if (gpx_datasource_.get() == NULL)
				gpx_datasource_.reset(new PreloadedGPXDatasource);

			std::cout << "GPX working " << std::endl;
			//https://api.openstreetmap.org/api/0.6/trackpoints?bbox=11.2,64.52,65,11.68&page=0

			Timer t;
			gpx_datasource_->Load(argv[narg]);
			fprintf(stderr, "Loaded in %.3f seconds\n", t.Count());
		} else {
			fprintf(stderr, "Not loading %s - unknown file type\n", argv[narg]);
		}
	}

	if (srtmpath) {
		heightmap_datasource_.reset(new SRTMDatasource(srtmpath));
		viewer_->SetHeightmapDatasource(heightmap_datasource_.get());
	} else {
		heightmap_datasource_.reset(new DummyHeightmap());
	}

	if (osm_datasource_.get() == NULL)
		throw Exception() << "no osm dump specified";

	gettimeofday(&curtime_, NULL);
	prevtime_ = curtime_;
	fpstime_ = curtime_;
}

void GlosmViewer::InitGL() {
	ShowCursor(!mouse_capture_);

#if defined(WITH_GLEW)
	GLenum err = glewInit();
	if (err != GLEW_OK)
		throw Exception() << "Cannot init glew: " << glewGetErrorString(err);
	const char *gl_requirements = "GL_VERSION_1_5";
	if (!glewIsSupported(gl_requirements)) {
		Exception e;
		e << "Minimal OpenGL requirements (" << gl_requirements << ") not met, unable to continue";
		if (no_glew_check_)
			fprintf(stderr, "(ignored) %s\n", e.what());
		else
			throw e;
	}
#endif
	CheckGL();

	geometry_generator_.reset(new GeometryGenerator(*osm_datasource_, *heightmap_datasource_));
	ground_layer_.reset(new GeometryLayer(projection_, *geometry_generator_));

	ground_layer_->SetLevel(9);
	ground_layer_->SetRange(100000.0);
	ground_layer_->SetFlags(GeometryDatasource::GROUND);
	ground_layer_->SetHeightEffect(false);
	ground_layer_->SetSizeLimit(1024*1024*1024);

	if (gpx_datasource_.get()) {
		gpx_layer_.reset(new GPXLayer(projection_, *gpx_datasource_, *heightmap_datasource_));
		gpx_layer_->SetLevel(10);
		gpx_layer_->SetRange(100000.0);
		gpx_layer_->SetHeightEffect(true);
		gpx_layer_->SetSizeLimit(32*1024*1024);
	}

	Vector3i startpos = geometry_generator_->GetCenter();
	osmint_t startheight = fabs((float)geometry_generator_->GetBBox().top - (float)geometry_generator_->GetBBox().bottom) / GEOM_LONSPAN * WGS84_EARTH_EQ_LENGTH * GEOM_UNITSINMETER / 10.0;
	float startyaw = 0;
	float startpitch = -M_PI_4;

	if (!std::isnan(start_lon_))
		startpos.x = start_lon_ * GEOM_UNITSINDEGREE;
	if (!std::isnan(start_lat_))
		startpos.y = start_lat_ * GEOM_UNITSINDEGREE;
	if (!std::isnan(start_ele_))
		startheight = start_ele_ * GEOM_UNITSINMETER;
	if (!std::isnan(start_yaw_))
		startyaw = start_yaw_;
	if (!std::isnan(start_pitch_))
		startpitch = start_pitch_;

	viewer_->SetPos(Vector3i(startpos, startheight));
	viewer_->SetRotation(startyaw, startpitch);
#if defined(WITH_TOUCHPAD)
	lockheight_ = startheight;
#endif
}

void GlosmViewer::Render() {
	/* update scene */
	gettimeofday(&curtime_, NULL);
	float dt = (float)(curtime_.tv_sec - prevtime_.tv_sec) + (float)(curtime_.tv_usec - prevtime_.tv_usec)/1000000.0f;

	/* render frame */
	glClearColor(0.1, 0.3, 0.5, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (ground_shown_) {
		ground_layer_->GarbageCollect();
		ground_layer_->LoadLocality(*viewer_);
		ground_layer_->Render(*viewer_);
	}

	if (gpx_shown_ && gpx_layer_.get()) {
		gpx_layer_->GarbageCollect();
		gpx_layer_->LoadLocality(*viewer_);
		gpx_layer_->Render(*viewer_);
	}

	glFlush();
	Flip();

	/* movement */
	if (movementflags_) {
		float myspeed = speed_;
		float height = viewer_->MutablePos().z / GEOM_UNITSINMETER;

		/* don't scale down under 100 meters */
		if (height > 100.0)
			myspeed *= height / 100.0;

		if (fast_)
			myspeed *= 5.0;
		if (slow_)
			myspeed /= 5.0;

		viewer_->Move(movementflags_, myspeed, dt);
	}
	if (lockheight_ != 0)
		viewer_->MutablePos().z = lockheight_;

	/* update FPS */
	float fpst = (float)(curtime_.tv_sec - fpstime_.tv_sec) + (float)(curtime_.tv_usec - fpstime_.tv_usec)/1000000.0f;

	if (fpst > 10.0) {
		fprintf(stderr, "FPS: %.3f\n", (float)nframes_/fpst);
		fpstime_ = curtime_;
		nframes_ = 0;
	}

	prevtime_ = curtime_;
	nframes_++;

#if !defined(DEBUG_FPS)
	/* frame limiter */
	usleep(10000);
#endif
}

void GlosmViewer::Resize(int w, int h) {
	if (w <= 0)
		w = 1;
	if (h <= 0)
		h = 1;

	screenw_ = w;
	screenh_ = h;

	float wanted_fov = 70.0f/180.0f*M_PI;
	float wanted_aspect = 4.0f/3.0f;

	float fov, aspect;

	if ((float)w/(float)h > wanted_aspect) { // wider than wanted
		fov = wanted_fov;
	} else { // narrower than wanted
		float wanted_h = (float)w / wanted_aspect;
		fov = 2.0f * atanf((float)h / wanted_h * tanf(wanted_fov/2.0f));
	}
	fov = wanted_fov;
	aspect = (float)w/(float)h;

	glViewport(0, 0, w, h);

	viewer_->SetFov(fov);
	viewer_->SetAspect(aspect);

	WarpCursor(w/2, h/2);
}

void GlosmViewer::KeyDown(int key) {
	switch (key) {
	case 27: case 'q':
		exit(0);
		break;
	case 'w': case KEY_UP:
		movementflags_ |= FirstPersonViewer::FORWARD;
		break;
	case 's': case KEY_DOWN:
		movementflags_ |= FirstPersonViewer::BACKWARD;
		break;
	case 'a': case KEY_LEFT:
		movementflags_ |= FirstPersonViewer::LEFT;
		break;
	case 'd': case KEY_RIGHT:
		movementflags_ |= FirstPersonViewer::RIGHT;
		break;
	case 'c':
		movementflags_ |= FirstPersonViewer::LOWER;
		break;
	case ' ':
		movementflags_ |= FirstPersonViewer::HIGHER;
		break;
	case 'l':
		lockheight_ = (lockheight_ == 0 ? viewer_->MutablePos().z : 0);
		break;
	case 'h':
		lockheight_ = (lockheight_ == 0 ? 1.75 * GEOM_UNITSINMETER : 0);
		break;
	case '+':
		speed_ *= 5.0f;
		break;
	case '-':
		speed_ /= 5.0f;
		break;
	case '1':
		ground_shown_ = !ground_shown_;
		break;
	case '3':
		gpx_shown_ = !gpx_shown_;
		break;
	case KEY_SHIFT:
		fast_ = true;
		break;
	case KEY_CTRL:
		slow_ = true;
		break;
	default:
		break;
	}
}

void GlosmViewer::KeyUp(int key) {
	switch (key) {
	case 'w': case KEY_UP:
		movementflags_ &= ~FirstPersonViewer::FORWARD;
		break;
	case 's': case KEY_DOWN:
		movementflags_ &= ~FirstPersonViewer::BACKWARD;
		break;
	case 'a': case KEY_LEFT:
		movementflags_ &= ~FirstPersonViewer::LEFT;
		break;
	case 'd': case KEY_RIGHT:
		movementflags_ &= ~FirstPersonViewer::RIGHT;
		break;
	case 'c':
		movementflags_ &= ~FirstPersonViewer::LOWER;
		break;
	case ' ':
		movementflags_ &= ~FirstPersonViewer::HIGHER;
		break;
	case KEY_SHIFT:
		fast_ = false;
		break;
	case KEY_CTRL:
		slow_ = false;
		break;
	default:
		break;
	}
}

void GlosmViewer::MouseMove(int x, int y) {
	if (drag_) {
		int dx = x - drag_start_pos_.x;
		int dy = y - drag_start_pos_.y;

		float yawdelta = -(float)dx / (float)screenw_ * viewer_->GetFov() * viewer_->GetAspect();
		float pitchdelta = (float)dy / (float)screenh_ * viewer_->GetFov();

		viewer_->SetRotation(drag_start_yaw_ + yawdelta, drag_start_pitch_ + pitchdelta);
	}

	if (mouse_capture_) {
		int dx = x - screenw_/2;
		int dy = y - screenh_/2;

		float yawdelta = (float)dx / 500.0;
		float pitchdelta = -(float)dy / 500.0;

		viewer_->Rotate(yawdelta, pitchdelta, 1.0);

		if (dx != 0 || dy != 0)
			WarpCursor(screenw_/2, screenh_/2);
	}
}

void GlosmViewer::MouseButton(int button, bool pressed, int x, int y) {
	if (button == BUTTON_RIGHT && pressed) {
		mouse_capture_ = !mouse_capture_;
		ShowCursor(!mouse_capture_);
	}

	if (!mouse_capture_ && button == BUTTON_LEFT) {
		drag_ = pressed;
		if (pressed) {
			drag_start_pos_.x = x;
			drag_start_pos_.y = y;
			drag_start_yaw_ = viewer_->GetYaw();
			drag_start_pitch_ = viewer_->GetPitch();
		}
	}
}
