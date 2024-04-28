// Copyright © 2023 Cory Petkovsek, Roope Palmroos, and Contributors.

#ifndef TERRAIN3D_MESH_INSTANCE_CLASS_H
#define TERRAIN3D_MESH_INSTANCE_CLASS_H

#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/texture2d.hpp>

#include "constants.h"
#include "terrain_3d_asset_resource.h"

using namespace godot;

class Terrain3DMeshInstance : public Terrain3DAssetResource {
	GDCLASS(Terrain3DMeshInstance, Terrain3DAssetResource);
	CLASS_NAME();
	friend class Terrain3DAssets;

private:
	Ref<PackedScene> _packed_scene;
	Node *_scene_node = nullptr;
	TypedArray<Mesh> _meshes;

public:
	Terrain3DMeshInstance();
	~Terrain3DMeshInstance();

	void clear();

	void set_name(String p_name);
	String get_name() const { return _name; }

	void set_id(int p_new_id);
	int get_id() const { return _id; }

	void set_scene_file(const Ref<PackedScene> p_scene_file);
	Ref<PackedScene> get_scene_file() const { return _packed_scene; }

	Ref<Texture2D> get_thumbnail() const;

protected:
	static void _bind_methods();
};

#endif // TERRAIN3D_MESH_INSTANCE_CLASS_H