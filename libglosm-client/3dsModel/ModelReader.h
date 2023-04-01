#pragma once 

#include "model3DS.h"

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
	struct model_w_skladzie* sklad_tmp = sklad_modeli;
	while (sklad_tmp){
		return sklad_tmp->model;
	}
	return NULL;
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

	model_tmp = new model3DS ("/home/pawlo/Pulpit/szablon - tunel/data/lawka.3ds",1,0);
	dodajModel (model_tmp,"lawka.3ds");
	printf("[3DS] Model '%s' stored\n","lawka.3ds");
}
