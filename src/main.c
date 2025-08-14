#include <psp2/ctrl.h>
#include <psp2/display.h>
#include <psp2/kernel/processmgr.h>
#include <vitaGL.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h> // abs()

#include "obj_loader.h"

#define DEADZONE 12       // мёртвая зона стика (из 0..255)
#define SENS     0.25f    // чувствительность вращения (градусы за тик)

static inline float clampf(float v, float lo, float hi){ return v<lo?lo:(v>hi?hi:v); }

int main(void) {
    // Инициализация ввода
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
    SceCtrlData pad;

    // Инициализация vitaGL (GPU память ~8 МБ — хватит для модели средних размеров)
    if (vglInit(8 * 1024 * 1024) != 0) {
        sceKernelExitProcess(1);
        return 1;
    }

    // Базовая настройка 3D
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glShadeModel(GL_SMOOTH);

    // Немного освещения
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    GLfloat light_pos[4] = { 0.0f, 1.0f, 1.0f, 0.0f }; // направленный свет
    GLfloat light_col[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_col);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_col);

    GLfloat mat_diff[4] = { 0.8f, 0.85f, 0.9f, 1.0f };
    GLfloat mat_spec[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  mat_diff);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_spec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 16.0f);

    // Проекция
    glViewport(0, 0, 960, 544);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspect = 960.0f / 544.0f;
    // приближённый perspective через frustum
    float f = 1.0f; // ~tan(fov/2)^-1, пусть будет 1
    glFrustumf(-aspect/f, aspect/f, -1.0f/f, 1.0f/f, 1.0f, 2000.0f);

    // Загрузим модель
    Mesh mesh = {0};
    if (load_obj_mesh("app0:/assets/DATNIGHTS-0.obj", &mesh) != 0 || mesh.vert_count == 0) {
        // Если не нашли файл — завершаемся аккуратно
        vglEnd();
        sceKernelExitProcess(2);
        return 2;
    }

    // Подготовим клиентские массивы
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);

    float ang_x = 0.0f, ang_y = 0.0f;
    float dist  = 3.0f; // дистанция камеры
    // Оценим масштаб модели, чтобы поместить в кадр (грубая нормализация)
    // Пробежимся по вершинам и найдём max |coord|
    float maxabs = 0.0f;
    for (size_t i = 0; i < mesh.vert_count * 3; ++i) {
        float a = fabsf(mesh.vertices[i]);
        if (a > maxabs) maxabs = a;
    }
    if (maxabs < 1e-6f) maxabs = 1.0f;
    float inv_scale = 1.0f / maxabs; // нормализуем так, чтобы модель ~[-1..1]
    dist = 3.0f;

    // Главный цикл
    for (;;) {
        sceCtrlPeekBufferPositive(0, &pad, 1);

        // Выход: START
        if (pad.buttons & SCE_CTRL_START) break;

        // Зум на триггерах
        if (pad.buttons & SCE_CTRL_RTRIGGER) dist -= 0.05f;
        if (pad.buttons & SCE_CTRL_LTRIGGER) dist += 0.05f;
        dist = clampf(dist, 1.0f, 20.0f);

        // Аналог: вращение
        int lx = (int)pad.lx - 128;
        int ly = (int)pad.ly - 128;
        if (abs(lx) > DEADZONE) ang_y += (float)lx * SENS * (1.0f/128.0f) * 3.0f; // yaw
        if (abs(ly) > DEADZONE) ang_x += (float)ly * SENS * (1.0f/128.0f) * 3.0f; // pitch
        if (ang_x >  89.9f) ang_x = 89.9f;
        if (ang_x < -89.9f) ang_x = -89.9f;

        // Рендер
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        // Камера: отодвинемся по Z, потом накрутим углы
        glTranslatef(0.0f, 0.0f, -dist);
        glRotatef(ang_x, 1.0f, 0.0f, 0.0f);
        glRotatef(ang_y, 0.0f, 1.0f, 0.0f);
        glScalef(inv_scale, inv_scale, inv_scale);

        // Расположим свет каждый кадр (для направленного не критично, но ок)
        glLightfv(GL_LIGHT0, GL_POSITION, light_pos);

        glVertexPointer(3, GL_FLOAT, 0, mesh.vertices);
        glNormalPointer(GL_FLOAT, 0, mesh.normals);

        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)mesh.vert_count);

        vglSwapBuffers(GL_FALSE); // без ожидания vblank — vgl сам синхронизирует
    }

    free_mesh(&mesh);
    vglEnd();
    sceKernelExitProcess(0);
    return 0;
}
