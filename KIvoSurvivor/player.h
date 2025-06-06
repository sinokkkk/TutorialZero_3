﻿#ifndef _PLAYER_H_
#define _PLAYER_H_

#include "timer.h"
#include "bullet.h"
#include "vector2.h"
#include "particle.h"

#include <vector>
#include <graphics.h>

extern Atlas atlas_run_effect;

extern std::vector<Bullet*> bullet_list;


extern bool is_debug;

class Player
{
public:
	Player(bool facing_right = true) : is_facing_right(facing_right)
	{
		current_animation = is_facing_right ? &animation_idle_right : &animation_idle_left;

		timer_invulnerable.set_wait_time(750);
		timer_invulnerable.set_one_shot(true);
		timer_invulnerable.set_callback([&]()
			{
				is_invulnerable = false;

			});//fucking lambda

		timer_invulnerable_blink.set_wait_time(75);
		timer_invulnerable_blink.set_callback([&]()
			{
				is_showing_sketch_frame = !is_showing_sketch_frame;
			});



		timer_run_effect_generation.set_wait_time(75);
		timer_run_effect_generation.set_callback([&]()
			{
				Vector2 particle_position;
				IMAGE* frame = atlas_run_effect.get_image(0);
				particle_position.x = position.x + (size.x - frame->getwidth()) / 2;
				particle_position.y = position.y + size.y - frame->getheight();
				particle_list.emplace_back(particle_position, &atlas_run_effect, 45);
			});

		timer_die_effect_generation.set_wait_time(35);
		timer_die_effect_generation.set_callback([&]()
			{
				Vector2 particle_position;
				IMAGE* frame = atlas_run_effect.get_image(0);
				particle_position.x = position.x + (size.x - frame->getwidth()) / 2;
				particle_position.y = position.y + size.y - frame->getheight();
				particle_list.emplace_back(particle_position, &atlas_run_effect, 150);
			});

		timer_attack_cd.set_wait_time(attack_cd);
		timer_attack_cd.set_one_shot(true);
		timer_attack_cd.set_callback([&]()
			{
				can_attack = true;
			});
	}

	~Player() = default;

	virtual void on_update(int delta){
		// 计算移动方向//数学，很奇妙吧
		int dir_x = is_right_key_down - is_left_key_down;
		int dir_y = is_down_key_down - is_up_key_down;
		
		if (dir_x != 0 || dir_y != 0) {
			if (!is_attacking_ex) {
				is_facing_right = dir_x > 0 || (dir_x == 0 && is_facing_right);
			}
			current_animation = is_facing_right ? &animation_run_right : &animation_run_left;

			// 计算方向向量长度并归一化
			float len_dir = sqrtf(dir_x * dir_x + dir_y * dir_y);
			float normalized_x = dir_x / len_dir;
			float normalized_y = dir_y / len_dir;

			// 应用移动
			position.x += normalized_x * run_velocity * delta;
			position.y += normalized_y * run_velocity * delta;

			// 立即进行边界检查
			if (position.x < 0) position.x = 0;
			if (position.y < 0) position.y = 0;
			if (position.x + size.x > getwidth()) position.x = getwidth() - size.x;
			if (position.y + size.y > getheight()) position.y = getheight() - size.y;

			timer_run_effect_generation.resume();
		}
		else {
			current_animation = is_facing_right ? &animation_idle_right : &animation_idle_left;
			timer_run_effect_generation.pause();
		}

        // 特殊攻击状态
        if (is_attacking_ex)
            current_animation = is_facing_right ? &animation_attack_ex_right : &animation_attack_ex_left;

        // 死亡状态
        if (hp <= 0)
            current_animation = last_hurt_direction.x < 0 ? &animation_die_left : &animation_die_right;

        // 更新动画
        current_animation->on_update(delta);

        // 更新计时器
        timer_attack_cd.on_update(delta);
        timer_invulnerable.on_update(delta);
        timer_invulnerable_blink.on_update(delta);
 		timer_run_effect_generation.on_update(delta);


		if (hp <= 0)
			timer_die_effect_generation.on_update(delta);

        // 处理无敌闪烁效果
        if (is_showing_sketch_frame)
            sketch_image(current_animation->get_frame(), &img_sketch);

		// 更新粒子效果
		particle_list.erase(std::remove_if(
			particle_list.begin(), particle_list.end(),
			[](const Particle& particle)
			{
				return !particle.check_valid();
			}),
			particle_list.end());
		for (Particle& particle : particle_list)
			particle.on_update(delta);

        // 处理移动和碰撞
        move_and_collide(delta);
    }


	virtual void on_draw(const Camera& camera){
		// 绘制粒子效果
		for (const Particle& particle : particle_list)
			particle.on_draw(camera);

		// 调试信息
		if (is_debug) {
			settextcolor(RGB(255, 0, 0));  // 设置红色文本
			settextstyle(20, 0, _T("IPix"));  // 设置字体
			TCHAR debug_info[256];
			_stprintf_s(debug_info, _T("无敌状态: %s, 闪烁: %s, HP: %d"),
				is_invulnerable ? _T("是") : _T("否"),
				is_showing_sketch_frame ? _T("是") : _T("否"),
				hp);
			outtextxy(10, 10, debug_info);
		}

		//绘制角色
		if (hp > 0 && is_invulnerable && is_showing_sketch_frame)//受伤闪烁逻辑
			putimage_alpha(camera, (int)position.x, (int)position.y, &img_sketch);
		else
			current_animation->on_draw(camera, (int)position.x, (int)position.y);

		//调试模式
		if (is_debug){
			setlinecolor(RGB(0, 125, 255));
			rectangle((int)position.x, (int)position.y, (int)(position.x + size.x), (int)(position.y + size.y));
		}
	}

	virtual void on_input(const ExMessage& msg){
		if (hp <= 0) return;//死了

		switch (msg.message)
		{
			case WM_KEYDOWN:
				switch (msg.vkcode)
				{
					// 'A'
				case 0x41:
					is_left_key_down = true;
					break;
					// 'D'
				case 0x44:
					is_right_key_down = true;
					break;
					// 'W'
				case 0x57:
					is_up_key_down = true;
					break;
					// 'S'
				case 0x53:
					is_down_key_down = true;
					break;
				}
				break;
			case WM_KEYUP:
				switch (msg.vkcode)
				{
					// 'A'
				case 0x41:
					is_left_key_down = false;
					break;
					// 'D'
				case 0x44:
					is_right_key_down = false;
					break;
					// 'W'
				case 0x57:
					is_up_key_down = false;
					break;
					// 'S'
				case 0x53:
					is_down_key_down = false;
					break;
				}
				break;
			case WM_LBUTTONDOWN:  // 左键按下
				if (can_attack)
				{
					on_attack();
					can_attack = false;
					timer_attack_cd.restart();
				}
				break;
			case WM_RBUTTONDOWN:  // 右键按下
				if (mp >= 100)
				{
					on_attack_ex();
					mp = 0;
				}
				break;

		}

		// 解决措施：疯狂添加按键状态检查
		if (GetAsyncKeyState(0x41) >= 0) is_left_key_down = false;    // A键
		if (GetAsyncKeyState(0x44) >= 0) is_right_key_down = false;   // D键
		if (GetAsyncKeyState(0x57) >= 0) is_up_key_down = false;      // W键
		if (GetAsyncKeyState(0x53) >= 0) is_down_key_down = false;    // S键
    }
	

    virtual void on_run(float dir_x, float dir_y){
        if (is_attacking_ex) return;//ex下不移动
        position.x += dir_x;
        position.y += dir_y;
    }


	virtual void on_attack() { }
	virtual void on_attack_ex() { }

	void set_hp(int val)	{ hp = val; }
	void set_mp(int val)	{ mp = val; }

	int get_hp() const		{return hp;}

	int get_mp() const		{return mp;}

	void set_position(float x, float y)		{position.x = x, position.y = y;}

	const Vector2& get_postion() const		{return position;}

	const Vector2& get_size() const			{return size;}

	bool is_invulnerable_state() const { return is_invulnerable; }  // 获取无敌状态
	void trigger_invulnerable() { make_invulnerable(); }  // 触发无敌状态

	// 添加getter和setter方法
	float get_attack_cd() const { return attack_cd; }
	void set_attack_cd(float cd) { attack_cd = cd; }

	float get_run_velocity() const { return run_velocity; }
	void set_run_velocity(float velocity) { run_velocity = velocity; }

	int get_max_hp() const { return hp; }
	void set_max_hp(int new_hp) { hp = new_hp; }

protected:
    float run_velocity = 0.30f;  // 移动速度
    int mp = 0;
    int hp = 100;
    int attack_cd = 500;

    IMAGE img_sketch;
    Vector2 size;
    Vector2 position;
    Vector2 velocity;
    Vector2 last_hurt_direction;

    bool can_attack = true;
    bool is_facing_right = true;
    bool is_left_key_down = false;
    bool is_right_key_down = false;
    bool is_up_key_down = false;
    bool is_down_key_down = false;
    bool is_attacking_ex = false;
    bool is_invulnerable = false;
    bool is_showing_sketch_frame = false;

    // 动画相关
    Animation animation_idle_left;
    Animation animation_idle_right;
    Animation animation_run_left;
    Animation animation_run_right;
    Animation animation_attack_ex_left;
    Animation animation_attack_ex_right;
    Animation animation_die_left;
    Animation animation_die_right;
    Animation* current_animation = nullptr;

    // 计时器
    Timer timer_attack_cd;
    Timer timer_invulnerable;
    Timer timer_invulnerable_blink;
	Timer timer_run_effect_generation;
	Timer timer_die_effect_generation;
	std::vector<Particle> particle_list;

protected:
	void make_invulnerable()
	{
		is_invulnerable = true;
		timer_invulnerable.restart();
	}

	void move_and_collide(int delta){
		if (!is_invulnerable){// 如果当前处于无敌状态，则不检测子弹碰撞
			
			for (Bullet* bullet : bullet_list){// 遍历所有子弹
				if (!bullet->get_valid() || bullet->get_owner() == this ) continue;// 跳过无效子弹或目标不是本玩家的子弹
				
				if (bullet->check_collision(position, size)){// 检查子弹是否与玩家发生碰撞
					make_invulnerable(); // 进入无敌状态，防止连续受伤
					bullet->on_collide(); // 触发子弹的碰撞效果（如爆炸等）
					bullet->set_valid(false); // 子弹失效（消失）
					hp -= bullet->get_damage(); // 扣除玩家生命值
					last_hurt_direction = bullet->get_position() - position; // 记录受伤方向（用于死亡动画朝向）

					// 如果玩家生命值小于等于0，设置死亡时的击退效果
					if (hp <= 0){
						velocity.x = last_hurt_direction.x < 0 ? 0.35f : -0.35f; // 根据受击方向设置水平击退
						velocity.y = -1.0f; // 设置垂直向上的击退
					}
				}
			}
		}
	}
};

#endif // !_PLAYER_H_
