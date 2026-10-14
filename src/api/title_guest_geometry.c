/* Decode RenderWare collision and title trigger records on the title thread.
 * Bounds, recursion and work budgets are part of this adapter's contract.
 * The recovery references are recorded in REVERSE_ENGINEERING.md. */
#define RECOMP_GENERATED_CODE
#include "gen/recomp_types.h"
#include "title_guest_geometry.h"
#include "title_guest_state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
SRWLOCK geometry_lock = SRWLOCK_INIT;
TitleGeometrySnapshot title_geometry_snapshot;
static uint32_t cache_world, cache_scene, cache_root;

static int guest_range_readable(uint32_t a, size_t n)
{
    return a >= 0x10000 && a < 0x4000000 && n <= 0x4000000 - a;
}

static Vector3 read_guest_vector(uint32_t a)
{
    Vector3 v = {MEMF(a), MEMF(a + 4), MEMF(a + 8)};
    return v;
}

/* Original title xScene: trig count +8, trig list +28, xEnv +40.
 * iEnv.collision is +4. RpWorld root +1C; leaf type -1, polygon +4,
 * vertices +8, numVertices +8A, numPolygons +8C; polygon is mat,a,b,c. */
static int copy_collision_sector(uint32_t sector, unsigned recursion, unsigned *budget)
{
    if (!guest_range_readable(sector, 0x90) || recursion > 64 || !*budget)
    {
        return 0;
    }
    --*budget;
    int type = (int32_t)MEM32(sector);
    if (type != -1)
    {
        if (type != 0 && type != 4 && type != 8)
        {
            return 0;
        }
        int a = copy_collision_sector(MEM32(sector + 8), recursion + 1, budget);
        int b = copy_collision_sector(MEM32(sector + 12), recursion + 1, budget);
        return a && b;
    }
    unsigned nv = MEM16(sector + 0x8A), nt = MEM16(sector + 0x8C);
    uint32_t verts = MEM32(sector + 8), indices = MEM32(sector + 4);
    if (!nv || !nt)
    {
        return 1;
    }
    if (!guest_range_readable(verts, (size_t)nv * 12) || !guest_range_readable(indices, (size_t)nt * 8) ||
        nt > MAX_TRIANGLES - title_geometry_snapshot.triangle_count)
    {
        return 0;
    }
    for (unsigned i = 0; i < nt; i++)
    {
        uint32_t p = indices + i * 8;
        unsigned ix[3] = {MEM16(p + 2), MEM16(p + 4), MEM16(p + 6)};
        if (ix[0] >= nv || ix[1] >= nv || ix[2] >= nv)
        {
            return 0;
        }
        CollisionTriangle t;
        for (int j = 0; j < 3; j++)
        {
            t.p[j] = read_guest_vector(verts + ix[j] * 12);
            if (!vector_is_valid(t.p[j]))
            {
                return 0;
            }
        }
        Vector3 a = vector_subtract(t.p[1], t.p[0]), b = vector_subtract(t.p[2], t.p[0]);
        Vector3 n = {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
        float length = sqrtf(vector_dot(n, n));
        float up = length > 0 ? fabsf(n.y) / length : 0;
        unsigned light = (unsigned)(70 + 130 * up);
        t.color = 0xFF000000 | (light / 2 << 16) | (light << 8) | (light + 30);
        title_geometry_snapshot.triangles[title_geometry_snapshot.triangle_count++] = t;
    }
    return 1;
}

static uint32_t find_scene_object(uint32_t scene, uint32_t id)
{
    unsigned n = MEM32(scene + 0x74);
    uint32_t list = MEM32(scene + 0x78);
    if (n > 8192 || !guest_range_readable(list, n * 4))
    {
        return 0;
    }
    for (unsigned i = 0; i < n; i++)
    {
        uint32_t a = MEM32(list + i * 4);
        if (guest_range_readable(a, 0x10) && MEM32(a) == id)
        {
            return a;
        }
    }
    return 0;
}

/* Follow a small number of relay links as many trigger volumes target a
 * dispatcher rather than the portal directly. Never invoke events. */
static unsigned classify_trigger_links(uint32_t scene, uint32_t base, unsigned limit, unsigned *budget)
{
    if (!guest_range_readable(base, 16) || !limit || !*budget)
    {
        return 0;
    }
    --*budget;
    unsigned n = MEM8(base + 5);
    uint32_t links = MEM32(base + 8);
    unsigned kind = 0;
    if (!guest_range_readable(links, n * 32))
    {
        return 0;
    }
    for (unsigned i = 0; i < n; i++)
    {
        uint32_t l = links + i * 32;
        unsigned e = MEM16(l + 2);
        if (e == 0x33 || e == 0x5D)
        {
            kind = 1; /* PlayerDeath / FallToDeath */
        }
        uint32_t target = find_scene_object(scene, MEM32(l + 4));
        if (e == 0x10 && guest_range_readable(target, 16) && MEM32(target + 12) == 0x73720)
        {
            kind = 2; /* portal teleport event */
        }
        if (target == 0x1BEF20 && (e == 0x24 || e == 0x25))
        {
            kind = 1;
        }
        unsigned next = classify_trigger_links(scene, target, limit - 1, budget);
        if (next == 1)
        {
            return 1;
        }
        if (next == 2)
        {
            kind = 2;
        }
    }
    return kind;
}

static void copy_triggers(uint32_t scene)
{
    title_geometry_snapshot.trigger_count = 0;
    unsigned n = MEM16(scene + 8);
    uint32_t list = MEM32(scene + 0x28);
    if (n > MAX_TRIGGERS || !guest_range_readable(list, n * 4))
    {
        return;
    }
    for (unsigned i = 0; i < n; i++)
    {
        uint32_t ent = MEM32(list + i * 4);
        if (!guest_range_readable(ent, 0x40))
        {
            continue;
        }
        uint32_t asset = MEM32(ent + 16);
        if (!guest_range_readable(asset, 0x90))
        {
            continue;
        }
        unsigned shape = MEM8(ent + 0x15);
        Vector3 low, high;
        if (shape == 0)
        {
            low = read_guest_vector(asset + 0x50);
            high = read_guest_vector(asset + 0x5C);
        }
        else
        {
            Vector3 center = read_guest_vector(asset + 0x50);
            float radius = MEMF(asset + 0x5C);
            float halfheight = shape == 2 ? MEMF(asset + 0x60) * 0.5f : radius;
            low = (Vector3){center.x - radius, center.y - halfheight, center.z - radius};
            high = (Vector3){center.x + radius, center.y + halfheight, center.z + radius};
        }
        if (shape > 3 || !vector_is_valid(low) || !vector_is_valid(high))
        {
            continue;
        }
        Trigger *t = &title_geometry_snapshot.triggers[title_geometry_snapshot.trigger_count++];
        for (unsigned j = 0; j < 8; j++)
        {
            t->p[j] = (Vector3){j & 1 ? high.x : low.x, j & 2 ? high.y : low.y, j & 4 ? high.z : low.z};
        }
        unsigned budget = 64;
        unsigned kind = classify_trigger_links(scene, ent, 3, &budget);
        int enabled = MEM16(ent + 6) & 1;
        t->color = !enabled ? 0x606060 : kind == 1 ? 0xFF5050 : kind == 2 ? 0x40CFFF : 0xFFCF40;
        snprintf(t->label, sizeof t->label, "%s%s%s [%08X]",
                 kind == 1   ? "Death trigger"
                 : kind == 2 ? "Level load trigger"
                             : "Trigger",
                 shape ? " (bounds)" : "", enabled ? "" : " OFF", MEM32(ent));
    }
}

void title_guest_geometry_capture(int show_triggers, int show_collision)
{
    if (!show_triggers && !show_collision)
    {
        return;
    }
    AcquireSRWLockExclusive(&geometry_lock);
    title_geometry_snapshot.camera_frame_valid = 0;
    uint32_t mode = title_state_menu_mode(), scene = title_state_current_scene(),
             cam = title_state_active_camera();
    /* Native pause/map UI is already in the framebuffer; do not paint over it. */
    if (mode != 11 || !guest_range_readable(scene, 0x7C) || !guest_range_readable(cam, 0x88))
    {
        goto done;
    }
    uint32_t frame = MEM32(cam + 4);
    if (!guest_range_readable(frame, 0x90))
    {
        goto done;
    }
    title_geometry_snapshot.camera_right = read_guest_vector(frame + 0x50);
    title_geometry_snapshot.camera_up = read_guest_vector(frame + 0x60);
    title_geometry_snapshot.camera_at = read_guest_vector(frame + 0x70);
    title_geometry_snapshot.camera_position = read_guest_vector(frame + 0x80);
    title_geometry_snapshot.view_x = MEMF(cam + 0x68);
    title_geometry_snapshot.view_y = MEMF(cam + 0x6C);
    if (!vector_is_valid(title_geometry_snapshot.camera_position) ||
        !vector_is_valid(title_geometry_snapshot.camera_right) ||
        !vector_is_valid(title_geometry_snapshot.camera_up) ||
        !vector_is_valid(title_geometry_snapshot.camera_at) || !isfinite(title_geometry_snapshot.view_x) ||
        !isfinite(title_geometry_snapshot.view_y) || title_geometry_snapshot.view_x < 0.001f ||
        title_geometry_snapshot.view_y < 0.001f)
    {
        goto done;
    }
    if (show_triggers)
    {
        copy_triggers(scene);
    }
    else
    {
        title_geometry_snapshot.trigger_count = 0;
    }
    if (show_collision)
    {
        uint32_t env = MEM32(scene + 0x40), geom = guest_range_readable(env, 4) ? MEM32(env) : 0;
        uint32_t world = guest_range_readable(geom, 8) ? MEM32(geom + 4) : 0;
        uint32_t root = guest_range_readable(world, 0x20) ? MEM32(world + 0x1C) : 0;
        uint32_t id = MEM32(scene + 0x6C);
        if (world != cache_world || root != cache_root || id != cache_scene ||
            !title_geometry_snapshot.triangles)
        {
            if (!title_geometry_snapshot.triangles)
            {
                title_geometry_snapshot.triangles =
                    calloc(MAX_TRIANGLES, sizeof *title_geometry_snapshot.triangles);
            }
            title_geometry_snapshot.triangle_count = 0;
            unsigned budget = 8192;
            int ok = title_geometry_snapshot.triangles && root && copy_collision_sector(root, 0, &budget);
            if (!ok)
            {
                title_geometry_snapshot.triangle_count = 0;
            }
            cache_world = world;
            cache_root = root;
            cache_scene = id;
            fprintf(stderr,
                    "[DEBUG-GEOMETRY] Collision scene=%08X world=%08X triangles=%u "
                    "valid=%d\n",
                    id, world, title_geometry_snapshot.triangle_count, ok);
        }
    }
    title_geometry_snapshot.camera_frame_valid = 1;
done:
    ReleaseSRWLockExclusive(&geometry_lock);
}

void title_guest_geometry_invalidate(void)
{
    AcquireSRWLockExclusive(&geometry_lock);
    cache_world = cache_root = cache_scene = 0;
    ReleaseSRWLockExclusive(&geometry_lock);
}
