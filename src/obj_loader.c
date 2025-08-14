#include "obj_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct { float x, y, z; } vec3;

static void add_vec3(vec3 **arr, size_t *count, size_t *cap, vec3 v) {
    if (*count + 1 > *cap) {
        *cap = (*cap == 0) ? 128 : (*cap * 2);
        *arr = (vec3*)realloc(*arr, (*cap) * sizeof(vec3));
    }
    (*arr)[(*count)++] = v;
}

static void add_float(float **arr, size_t *count, size_t *cap, float v) {
    if (*count + 1 > *cap) {
        *cap = (*cap == 0) ? 256 : (*cap * 2);
        *arr = (float*)realloc(*arr, (*cap) * sizeof(float));
    }
    (*arr)[(*count)++] = v;
}

static void face_normal(const float *a, const float *b, const float *c, float *nx, float *ny, float *nz) {
    float ux = b[0] - a[0], uy = b[1] - a[1], uz = b[2] - a[2];
    float vx = c[0] - a[0], vy = c[1] - a[1], vz = c[2] - a[2];
    float x = uy * vz - uz * vy;
    float y = uz * vx - ux * vz;
    float z = ux * vy - uy * vx;
    float len = sqrtf(x*x + y*y + z*z);
    if (len < 1e-8f) len = 1.0f;
    *nx = x / len; *ny = y / len; *nz = z / len;
}

int load_obj_mesh(const char *path, Mesh *out) {
    memset(out, 0, sizeof(*out));
    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    vec3 *tmp_pos = NULL, *tmp_nrm = NULL;
    size_t n_pos = 0, cap_pos = 0, n_nrm = 0, cap_nrm = 0;

    float *v_final = NULL, *n_final = NULL;
    size_t n_vfinal = 0, cap_vfinal = 0, n_nfinal = 0, cap_nfinal = 0;

    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == 'v' && line[1] == ' ') {
            vec3 v; if (sscanf(line+2, "%f %f %f", &v.x, &v.y, &v.z) == 3) {
                add_vec3(&tmp_pos, &n_pos, &cap_pos, v);
            }
        } else if (line[0] == 'v' && line[1] == 'n') {
            vec3 n; if (sscanf(line+2, "%f %f %f", &n.x, &n.y, &n.z) == 3) {
                add_vec3(&tmp_nrm, &n_nrm, &cap_nrm, n);
            }
        } else if (line[0] == 'f' && line[1] == ' ') {
            // поддерживаем форматы: v/vt/vn, v//vn, v
            // токенизируем по пробелам
            char *p = line + 2;
            char *tokens[16]; int tcount = 0;
            while (*p && tcount < 16) {
                while (*p == ' ') p++;
                if (!*p || *p=='\n' || *p=='\r') break;
                tokens[tcount++] = p;
                while (*p && *p!=' ' && *p!='\n' && *p!='\r') p++;
                if (*p) { *p = '\0'; p++; }
            }
            if (tcount < 3) continue;

            // Фан‑триангуляция: (0, i-1, i)
            for (int i = 2; i < tcount; ++i) {
                int idxs[3] = {0, i-1, i};
                float tri_pos[9];
                float tri_nrm[9];
                int have_normals = 1;

                for (int k = 0; k < 3; ++k) {
                    int vi = 0, ni = 0;
                    char *tok = tokens[idxs[k]];
                    // разбираем v/vt/vn | v//vn | v
                    // заменим все '/' на ' ' и прочтём числа
                    char buf[64]; strncpy(buf, tok, sizeof(buf)-1); buf[sizeof(buf)-1] = 0;
                    int slashes = 0; for (char *c=buf; *c; ++c) if (*c=='/') { *c=' '; slashes++; }
                    if (slashes == 0) {
                        // "v"
                        if (sscanf(buf, "%d", &vi) != 1) { vi = 0; }
                        ni = 0;
                    } else if (slashes == 1) {
                        // "v vt" (нормалей нет)
                        if (sscanf(buf, "%d %*d", &vi) != 1) vi = 0;
                        ni = 0; have_normals = 0;
                    } else {
                        // "v vt vn" или "v  vn"
                        // читаем три числа, но vt можно пропустить через %*d
                        int read = sscanf(buf, "%d %*d %d", &vi, &ni);
                        if (read < 1) vi = 0;
                        if (read < 2) { ni = 0; have_normals = 0; }
                    }
                    if (vi < 0) vi = (int)n_pos + 1 + vi; // отрицательные не обяз., но поддержим
                    if (ni < 0) ni = (int)n_nrm + 1 + ni;

                    if (vi <= 0 || vi > (int)n_pos) goto fail;
                    vec3 vp = tmp_pos[vi-1];
                    tri_pos[k*3+0] = vp.x; tri_pos[k*3+1] = vp.y; tri_pos[k*3+2] = vp.z;

                    if (have_normals && ni > 0 && ni <= (int)n_nrm) {
                        vec3 vn = tmp_nrm[ni-1];
                        tri_nrm[k*3+0] = vn.x; tri_nrm[k*3+1] = vn.y; tri_nrm[k*3+2] = vn.z;
                    } else {
                        have_normals = 0;
                    }
                }

                if (!have_normals) {
                    float nx, ny, nz;
                    face_normal(&tri_pos[0], &tri_pos[3], &tri_pos[6], &nx, &ny, &nz);
                    for (int k = 0; k < 3; ++k) {
                        tri_nrm[k*3+0] = nx;
                        tri_nrm[k*3+1] = ny;
                        tri_nrm[k*3+2] = nz;
                    }
                }

                // добавить в конечные массивы
                for (int j = 0; j < 9; ++j) add_float(&v_final, &n_vfinal, &cap_vfinal, tri_pos[j]);
                for (int j = 0; j < 9; ++j) add_float(&n_final, &n_nfinal, &cap_nfinal, tri_nrm[j]);
            }
        }
    }
    fclose(f);

    out->vertices = v_final;
    out->normals  = n_final;
    out->vert_count = n_vfinal / 3;
    free(tmp_pos);
    free(tmp_nrm);
    return 0;

fail:
    fclose(f);
    free(tmp_pos); free(tmp_nrm);
    free(v_final); free(n_final);
    memset(out, 0, sizeof(*out));
    return -2;
}

void free_mesh(Mesh *m) {
    if (!m) return;
    free(m->vertices);
    free(m->normals);
    memset(m, 0, sizeof(*m));
}
