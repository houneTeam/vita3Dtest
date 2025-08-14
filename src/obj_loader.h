#pragma once
#include <stddef.h>

typedef struct {
    float *vertices;   // xyzxyz...
    float *normals;    // nxnynz...
    size_t vert_count; // количество вершин (а не float-ов): vertices имеет 3*vert_count элементов
} Mesh;

// Загружает OBJ (только треугольники, позитивные индексы, v/vt/vn или v//vn).
// Если нормали в файле отсутствуют — посчитает покадровые нормали (по треугольникам).
// Возвращает 0 при успехе.
int load_obj_mesh(const char *path, Mesh *out);

// Освободить память
void free_mesh(Mesh *m);
