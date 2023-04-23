#pragma once 

#include "model3DS.h"

model3DS * model2;

struct model_w_skladzie {
	char * filename;
	model3DS * model;
	struct model_w_skladzie *wsk;
};
struct model_w_skladzie* sklad_modeli = NULL;

void dodajModel (model3DS * _model, char* file_name)
{
    struct model_w_skladzie* tmp;
    tmp = (struct model_w_skladzie *) malloc (sizeof(struct model_w_skladzie));
    tmp -> filename  = (char *) malloc(strlen(file_name)+1);
    strcpy( tmp -> filename, file_name);
    tmp -> model = _model;
    tmp->wsk = sklad_modeli;
    sklad_modeli = tmp;
}

model3DS * pobierzModel (char* file_name)
{
	//struct model_w_skladzie* sklad_tmp = sklad_modeli;
	//while (sklad_tmp){
	//return sklad_tmp->model;
	//}
	//return NULL;

	return model2;
}

void rysujModel(char * file_name)
{
	model3DS * model_tmp;	
	if (model_tmp = pobierzModel (file_name)){
		model_tmp -> draw();
	}
}

void ladujModele()
{
	model3DS * model_tmp;

	//model_tmp = new model3DS ("/home/pawlo/Pulpit/glosm/libglosm-client/data/BISMARK_.3DS",1,0);
	//dodajModel (model_tmp,"BISMARK_");
	//printf("[3DS] Model '%s' stored\n","BISMARK_");

	//model2 = new model3DS ("/home/pawlo/Pulpit/glosm/libglosm-client/data/BISMARK_.3DS",1,0);

	model_tmp = new model3DS ("/home/pawlo/Pulpit/glosm/libglosm-client/data/lawka.3ds",1,0);
	dodajModel (model_tmp,"lawka");
	printf("[3DS] Model '%s' stored\n","lawka");

	model2 = new model3DS ("/home/pawlo/Pulpit/glosm/libglosm-client/data/GLORIEUX.3DS",1,0);
}
