/* Render copied geometry on the window thread. Guest layout recovery and
 * scene traversal belong to title_guest_geometry.c. */
#include "api/title_guest_geometry.h"
#include "title_debug_geometry.h"
#include "debug_options.h"
#include <xbox/debug_overlay.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static volatile LONG show_triggers, show_collision;
static uint32_t *pixels;
static float *depth_buffer;
static size_t pixel_capacity;

static Vector3 world_to_camera(Vector3 v)
{
    v = vector_subtract(v, title_geometry_snapshot.camera_position);
    Vector3 p = {vector_dot(v, title_geometry_snapshot.camera_right),
                 vector_dot(v, title_geometry_snapshot.camera_up),
                 vector_dot(v, title_geometry_snapshot.camera_at)};
    return p;
}

void title_debug_geometry_frame(void)
{
    if (title_options.debug)
    {
        title_guest_geometry_capture(show_triggers, show_collision);
    }
}

int title_debug_geometry_enabled(int collision)
{
    return collision ? show_collision : show_triggers;
}

void title_debug_geometry_toggle(int collision)
{
    volatile LONG *flag = collision ? &show_collision : &show_triggers;
    InterlockedExchange(flag, !*flag);
    /* Force recopy even if the scene allocator reuses its previous addresses. */
    if (collision)
    {
        title_guest_geometry_invalidate();
    }
}

/* Clip against the near plane before perspective division. Bounding raster
 * coordinates are clamped to the framebuffer; debug geometry cannot issue
 * unbounded GDI operations when the camera enters a trigger. */
static int clip_to_near_plane(const Vector3 *p, int n, Vector3 *out)
{
    int count = 0;
    Vector3 a = p[n - 1];
    int ai = a.z >= 0.1f;
    for (int i = 0; i < n; i++)
    {
        Vector3 b = p[i];
        int bi = b.z >= 0.1f;
        if (ai != bi)
        {
            out[count++] = vector_lerp(a, b, (0.1f - a.z) / (b.z - a.z));
        }
        if (bi)
        {
            out[count++] = b;
        }
        a = b;
        ai = bi;
    }
    return count;
}

/* RenderWare's camera-frame horizontal basis points toward screen left. */
static Vector3 project_to_screen(Vector3 p, unsigned w, unsigned h)
{
    return (Vector3){w * (0.5f - p.x / (2 * title_geometry_snapshot.view_x * p.z)),
                     h * (0.5f - p.y / (2 * title_geometry_snapshot.view_y * p.z)), 1 / p.z};
}

static float triangle_edge(Vector3 a, Vector3 b, float x, float y)
{
    return (x - a.x) * (b.y - a.y) - (y - a.y) * (b.x - a.x);
}

static void rasterize_triangle(Vector3 a, Vector3 b, Vector3 c, unsigned color, int transparent, unsigned w,
                               unsigned h)
{
    a = project_to_screen(a, w, h);
    b = project_to_screen(b, w, h);
    c = project_to_screen(c, w, h);
    float area = triangle_edge(a, b, c.x, c.y);
    if (!isfinite(area) || fabsf(area) < 0.01f)
    {
        return;
    }
    float fx0 = fmaxf(0, fminf(a.x, fminf(b.x, c.x))), fy0 = fmaxf(0, fminf(a.y, fminf(b.y, c.y)));
    float fx1 = fminf((float)w - 1, fmaxf(a.x, fmaxf(b.x, c.x))),
          fy1 = fminf((float)h - 1, fmaxf(a.y, fmaxf(b.y, c.y)));
    if (fx0 > fx1 || fy0 > fy1)
    {
        return;
    }
    for (int y = (int)fy0; y <= (int)fy1; y++)
    {
        for (int x = (int)fx0; x <= (int)fx1; x++)
        {
            float u = triangle_edge(b, c, x + 0.5f, y + 0.5f) / area,
                  v = triangle_edge(c, a, x + 0.5f, y + 0.5f) / area, t = 1 - u - v;
            if (u < 0 || v < 0 || t < 0)
            {
                continue;
            }
            size_t i = (size_t)y * w + x;
            float z = u * a.z + v * b.z + t * c.z;
            if (!transparent)
            {
                if (z < depth_buffer[i])
                {
                    continue;
                }
                depth_buffer[i] = z;
                pixels[i] = (u < 0.015f || v < 0.015f || t < 0.015f) ? 0xFFB4D9E0 : color;
            }
            else
            {
                /* One translucent layer avoids darker boxes from face overlap. */
                if ((pixels[i] >> 24) == 255)
                {
                    uint32_t old = pixels[i];
                    unsigned r = (((color >> 16) & 255) * 48 + ((old >> 16) & 255) * 207) / 255;
                    unsigned g = (((color >> 8) & 255) * 48 + ((old >> 8) & 255) * 207) / 255;
                    unsigned b = ((color & 255) * 48 + (old & 255) * 207) / 255;
                    pixels[i] = 0xFF000000 | (r << 16) | (g << 8) | b;
                }
                else
                {
                    pixels[i] = 0x30000000 | (color & 0xFFFFFF);
                }
            }
        }
    }
}

static void draw_triangle(Vector3 a, Vector3 b, Vector3 c, unsigned color, int transparent, unsigned w,
                          unsigned h)
{
    Vector3 p[3] = {world_to_camera(a), world_to_camera(b), world_to_camera(c)}, q[4];
    int n = clip_to_near_plane(p, 3, q);
    for (int i = 1; i < n - 1; i++)
    {
        rasterize_triangle(q[0], q[i], q[i + 1], color, transparent, w, h);
    }
}

static void draw_geometry_overlay(void *device, unsigned w, unsigned h)
{
    if (!show_triggers && !show_collision)
    {
        return;
    }
    AcquireSRWLockShared(&geometry_lock);
    if (!title_geometry_snapshot.camera_frame_valid)
    {
        ReleaseSRWLockShared(&geometry_lock);
        return;
    }
    size_t count = (size_t)w * h;
    if (count > pixel_capacity)
    {
        free(pixels);
        free(depth_buffer);
        pixels = calloc(count, 4);
        depth_buffer = calloc(count, sizeof(float));
        pixel_capacity = (pixels && depth_buffer) ? count : 0;
    }
    if (!pixel_capacity)
    {
        ReleaseSRWLockShared(&geometry_lock);
        return;
    }
    for (size_t i = 0; i < count; i++)
    {
        pixels[i] = 0;
        depth_buffer[i] = 0;
    }
    if (show_collision)
    {
        for (unsigned i = 0; i < title_geometry_snapshot.triangle_count; i++)
        {
            CollisionTriangle *t = &title_geometry_snapshot.triangles[i];
            draw_triangle(t->p[0], t->p[1], t->p[2], t->color, 0, w, h);
        }
    }
    static const unsigned faces[6][4] = {{0, 1, 3, 2}, {4, 6, 7, 5}, {0, 4, 5, 1},
                                         {2, 3, 7, 6}, {0, 2, 6, 4}, {1, 5, 7, 3}};
    if (show_triggers)
    {
        for (unsigned i = 0; i < title_geometry_snapshot.trigger_count; i++)
        {
            for (int f = 0; f < 6; f++)
            {
                Trigger *t = &title_geometry_snapshot.triggers[i];
                const unsigned *v = faces[f];
                draw_triangle(t->p[v[0]], t->p[v[1]], t->p[v[2]], t->color, 1, w, h);
                draw_triangle(t->p[v[0]], t->p[v[2]], t->p[v[3]], t->color, 1, w, h);
            }
        }
    }
    /* Keep depth-tested collision faces as one half-opacity layer over the game. */
    if (show_collision)
    {
        for (size_t i = 0; i < count; i++)
        {
            if ((pixels[i] >> 24) == 255)
            {
                pixels[i] = 0x80000000 | (pixels[i] & 0xFFFFFF);
            }
        }
    }
    xbox_DebugWorldComposite(device, pixels, w, h);
    HDC dc = (HDC)device;
    int saved = SaveDC(dc);
    IntersectClipRect(dc, 0, 0, w, h);
    SetBkMode(dc, TRANSPARENT);
    if (show_triggers)
    {
        for (unsigned i = 0; i < title_geometry_snapshot.trigger_count; i++)
        {
            Trigger *t = &title_geometry_snapshot.triggers[i];
            HPEN pen = CreatePen(PS_SOLID, 1, RGB(t->color >> 16, (t->color >> 8) & 255, t->color & 255));
            HGDIOBJ old = SelectObject(dc, pen);
            for (unsigned j = 0; j < 8; j++)
            {
                for (unsigned bit = 1; bit <= 4; bit <<= 1)
                {
                    if (!(j & bit))
                    {
                        Vector3 a = world_to_camera(t->p[j]), b = world_to_camera(t->p[j | bit]);
                        if (a.z < 0.1f && b.z < 0.1f)
                        {
                            continue;
                        }
                        if (a.z < 0.1f)
                        {
                            a = vector_lerp(a, b, (0.1f - a.z) / (b.z - a.z));
                        }
                        if (b.z < 0.1f)
                        {
                            b = vector_lerp(b, a, (0.1f - b.z) / (a.z - b.z));
                        }
                        a = project_to_screen(a, w, h);
                        b = project_to_screen(b, w, h);
                        /* Reject pathological lines, while allowing viewport crossings. */
                        if (fabsf(a.x) > 100000 || fabsf(a.y) > 100000 || fabsf(b.x) > 100000 ||
                            fabsf(b.y) > 100000)
                        {
                            continue;
                        }
                        MoveToEx(dc, (int)a.x, (int)a.y, NULL);
                        LineTo(dc, (int)b.x, (int)b.y);
                    }
                }
            }
            SelectObject(dc, old);
            DeleteObject(pen);
            Vector3 anchor = vector_lerp(t->p[2], t->p[7], 0.5f);
            anchor.y = fmaxf(t->p[2].y, t->p[7].y) + 0.2f;
            anchor = world_to_camera(anchor);
            if (anchor.z < 0.1f)
            {
                continue;
            }
            anchor = project_to_screen(anchor, w, h);
            if (anchor.x < 0 || anchor.x >= w || anchor.y < 0 || anchor.y >= h)
            {
                continue;
            }
            SetTextColor(dc, RGB(t->color >> 16, (t->color >> 8) & 255, t->color & 255));
            TextOutA(dc, (int)anchor.x, (int)anchor.y, t->label, (int)strlen(t->label));
        }
    }
    if (show_collision)
    {
        SetTextColor(dc, RGB(180, 220, 240));
        char label[96];
        snprintf(label, sizeof label, "World collision: %u triangles%s",
                 title_geometry_snapshot.triangle_count,
                 title_geometry_snapshot.triangle_count ? "" : " (no collision mesh available)");
        TextOutA(dc, 12, (int)h - 20, label, (int)strlen(label));
    }
    RestoreDC(dc, saved);
    ReleaseSRWLockShared(&geometry_lock);
}

void title_debug_geometry_init(void)
{
    if (title_options.debug)
    {
        xbox_DebugWorldRegister(draw_geometry_overlay);
        xbox_DebugWorldFrameRegister(title_debug_geometry_frame);
    }
}
