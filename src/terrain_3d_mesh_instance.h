// Copyright © 2023 Cory Petkovsek, Roope Palmroos, and Contributors.

#ifndef TERRAIN3D_MESH_INSTANCE_CLASS_H
#define TERRAIN3D_MESH_INSTANCE_CLASS_H

#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource.hpp>

#include "constants.h"
#include "terrain_3d_asset_resource.h"

using namespace godot;

class Terrain3DMeshInstance : public Terrain3DAssetResource {
	GDCLASS(Terrain3DMeshInstance, Terrain3DAssetResource);
	CLASS_NAME();
	friend class Terrain3DAssets;

private:
	//String _name;
	//int _id = 0;
	Ref<PackedScene> _packed_scene;

	//bool _is_texture_valid(const Ref<Texture2D> &p_texture) const;

public:
	Terrain3DMeshInstance();
	~Terrain3DMeshInstance();

	// Edit data directly to avoid signal emitting recursion
	//Settings *get_data() { return &_data; }
	void clear();

	void set_name(String p_name);
	String get_name() const { return _name; }

	void set_id(int p_new_id);
	int get_id() const { return _id; }

	void set_scene(const Ref<PackedScene> p_scene);
	Ref<PackedScene> get_scene() const { return _packed_scene; }

	//void set_albedo_color(Color p_color);
	//Color get_albedo_color() const { return _data._albedo_color; }

	//void set_albedo_texture(const Ref<Texture2D> &p_texture);
	//Ref<Texture2D> get_albedo_texture() const { return _data._albedo_texture; }

	//void set_normal_texture(const Ref<Texture2D> &p_texture);
	//Ref<Texture2D> get_normal_texture() const { return _data._normal_texture; }

	//void set_uv_scale(real_t p_scale);
	//real_t get_uv_scale() const { return _data._uv_scale; }

	//void set_uv_rotation(real_t p_rotation);
	//real_t get_uv_rotation() const { return _data._uv_rotation; }

protected:
	static void _bind_methods();
};

#endif // TERRAIN3D_MESH_INSTANCE_CLASS_H