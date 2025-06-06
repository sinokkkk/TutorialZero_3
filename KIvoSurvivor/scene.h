﻿#ifndef _SCENE_H_
#define _SCENE_H_

#include "camera.h"

#include <graphics.h>

class Scene{
public:
	Scene() = default;
	~Scene() = default;

	virtual void on_enter() = 0;
	virtual void on_update(int delta) = 0;
	virtual void on_draw(const Camera& camera) = 0;
	virtual void on_input(const ExMessage& msg) = 0;
	virtual void on_exit() = 0;
	virtual void start_new_wave() {}
};


#endif // !_SCENE_H_
