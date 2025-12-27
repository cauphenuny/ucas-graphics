import bpy
import os
import math
import argparse
from mathutils import Vector


# ========= 路径 =========
OBJ_DIR = "/path/to/obj"
OUT_DIR = "/path/to/out"
os.makedirs(OUT_DIR, exist_ok=True)

# ========= 渲染参数 =========
RES = 1024

scene = bpy.context.scene
scene.render.engine = "CYCLES"
scene.render.image_settings.file_format = "PNG"
scene.render.film_transparent = True
scene.render.resolution_x = RES
scene.render.resolution_y = RES
scene.cycles.samples = 128
# Turn OFF Cycles denoising for file-rendering with transparency.
# Denoisers sometimes produce long dark streaks on alpha/transparent renders.
scene.cycles.use_denoising = False

# ========= 清空场景 =========
bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)

# ========= Camera =========
cam_data = bpy.data.cameras.new("Camera")
cam = bpy.data.objects.new("Camera", cam_data)
scene.collection.objects.link(cam)
scene.camera = cam

cam.location = Vector((-1.0, 2.0, 1.2))

# Point the camera to the scene origin robustly so orientation is correct
# regardless of which side it's placed on.
target = Vector((0.0, 0.0, 0.0))
dir_vec = target - cam.location
cam.rotation_euler = dir_vec.to_track_quat("-Z", "Y").to_euler()
cam.data.lens = 50


# ========= 灯光（经典三点光） =========
def add_light(name, loc, energy):
    light = bpy.data.lights.new(name, type="AREA")
    light.energy = energy
    obj = bpy.data.objects.new(name, light)
    obj.location = loc
    scene.collection.objects.link(obj)

    # Apply global shadow / softness settings
    try:
        light.use_shadow = ENABLE_SHADOWS
    except Exception:
        # older/newer API differences: ignore if property unavailable
        pass
    try:
        # area lights use `size` to control apparent source size (softness)
        light.size = SHADOW_SIZE
    except Exception:
        pass
    # scale energy so shadows/brightness are less strong if requested
    obj.data.energy = obj.data.energy * LIGHT_ENERGY_SCALE


add_light("Key", (3, -3, 4), 1200)
add_light("Fill", (-3, -2, 2), 600)
add_light("Back", (0, 3, 3), 400)


# ========= 归一化 =========
def normalize_objects(objs):
    for obj in objs:
        bpy.context.view_layer.objects.active = obj
        bpy.ops.object.origin_set(type="ORIGIN_GEOMETRY", center="BOUNDS")
    min_v = Vector((1e9, 1e9, 1e9))
    max_v = Vector((-1e9, -1e9, -1e9))
    for obj in objs:
        for v in obj.bound_box:
            w = obj.matrix_world @ Vector(v)
            min_v = Vector(map(min, min_v, w))
            max_v = Vector(map(max, max_v, w))
    center = (min_v + max_v) / 2
    scale = 1.0 / max((max_v - min_v))
    for obj in objs:
        obj.location -= center
        obj.scale *= scale


# ========= 阴影控制 =========
# 将下面的开关设为 False 可完全关闭由灯光产生的阴影，
# 或者通过增大 SHADOW_SIZE 来软化阴影（更大的光源尺寸产生更柔和的阴影）。
ENABLE_SHADOWS = False
SHADOW_SIZE = 2.0
LIGHT_ENERGY_SCALE = 0.8


# ========= 批处理 =========
for fname in sorted(os.listdir(OBJ_DIR)):
    if not fname.endswith(".obj"):
        continue
    # 删除旧 mesh
    bpy.ops.object.select_all(action="DESELECT")
    for o in scene.objects:
        if o.type == "MESH":
            o.select_set(True)
    bpy.ops.object.delete()
    # 导入 obj
    try:
        # use the standard OBJ import operator
        bpy.ops.wm.obj_import(filepath=os.path.join(OBJ_DIR, fname))
    except Exception as e:
        print(f"Failed to import {fname}: {e}")
        print("Skipping this file.")
        continue
    meshes = [o for o in scene.objects if o.type == "MESH"]
    normalize_objects(meshes)
    scene.render.filepath = os.path.join(OUT_DIR, fname.replace(".obj", ".png"))
    bpy.ops.render.render(write_still=True)

print("All done.")
