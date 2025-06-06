﻿#pragma once

#include "bullet.h"

extern IMAGE img_aris_bullet;        // 子弹贴图
extern Atlas atlas_aris_bullet_hit;  // 击中特效
extern Camera main_camera;

class ArisBullet : public Bullet
{
public:
    ArisBullet(void *owner=nullptr):Bullet(owner) //调用基类构造函数
    {
        // 基础属性设置
        size.x = 10, size.y = 10;  // 子弹大小
        damage = 10;  // 基础伤害
        speed = 2.0f;  // 基础速度
        
        // 设置贴图
        image = &img_aris_bullet;

        // 设置击中动画
        animation_hit.set_atlas(&atlas_aris_bullet_hit);
        animation_hit.set_interval(50);
        animation_hit.set_loop(false);
        animation_hit.set_callback([&]() { can_remove = true; });

        // 调试信息
        if (is_debug)
        {
            static TCHAR text[64];
            _stprintf_s(text, _T("ArisBullet 创建成功"));
            outtextxy(15, 15, text);
        }
    }

    void on_collide() override{
        // 不调用基类的on_collide，因为我们要实现贯穿效果  只播放击中音效和特效
        int random_sound = rand() % 3;  // 生成0-2的随机数，随机播放音效
        switch(random_sound)
        {
            case 0:
                mciSendString(_T("play aris_bullet_hit_1 from 0"), NULL, 0, NULL);
                break;
            case 1:
                mciSendString(_T("play aris_bullet_hit_2 from 0"), NULL, 0, NULL);
                break;
            case 2:
                mciSendString(_T("play aris_bullet_hit_3 from 0"), NULL, 0, NULL);
                break;
        }

        
        main_camera.shake(3, 100);
    }

	void on_update(int delta) override
	{
        position += velocity * (float)delta;
        if (!valid) animation_hit.on_update(delta);
        if (check_if_exceeds_screen()) can_remove = true;     // 检查是否超出屏幕
            

        // 调试信息
        if (is_debug)
        {
            static TCHAR text[64];
            _stprintf_s(text, _T("子弹位置: %.1f, %.1f"), position.x, position.y);
            outtextxy(15, 35, text);
        }
	}

    void on_draw(const Camera& camera) const
    {
        if (valid)
            putimage_alpha(camera, (int)position.x, (int)position.y, &img_aris_bullet);
        else
            animation_hit.on_draw(camera, (int)position.x, (int)position.y);

        Bullet::on_draw(camera);
    }



private:
    Animation animation_hit;   // 击中动画
};
