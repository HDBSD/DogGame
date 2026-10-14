#ifndef TITLE_GUEST_GEOMETRY_H
#define TITLE_GUEST_GEOMETRY_H
#include <windows.h>
#include "title_geometry_types.h"

typedef struct TitleGeometrySnapshot
{
    CollisionTriangle *triangles;
    unsigned triangle_count;
    Trigger triggers[MAX_TRIGGERS];
    unsigned trigger_count;
    Vector3 camera_right, camera_up, camera_at, camera_position;
    float view_x, view_y;
    int camera_frame_valid;
} TitleGeometrySnapshot;

/* Shared snapshot storage is protected by this lock. No guest pointers are
 * published: vectors, faces, labels and camera values have been copied. */
extern SRWLOCK geometry_lock;
extern TitleGeometrySnapshot title_geometry_snapshot;
void title_guest_geometry_capture(int show_triggers, int show_collision);
void title_guest_geometry_invalidate(void);
#endif
