// Copyright © 2023 Cory Petkovsek, Roope Palmroos, and Contributors.

#ifndef TERRAIN3D_ASSETS_CLASS_H
#define TERRAIN3D_ASSETS_CLASS_H

#include "constants.h"
#include "generated_texture.h"
#include "terrain_3d_mesh_instance.h"
#include "terrain_3d_texture.h"

using namespace godot;

class Terrain3DAssets : public Resource {
	GDCLASS(Terrain3DAssets, Resource);
	CLASS_NAME();

public: // Constants
	enum ResType {
		TYPE_TEXTURE,
		TYPE_MESH,
	};

	static inline const int MAX_TEXTURES = 32;
	static inline const int MAX_MESHES = 128;

private:
	TypedArray<Terrain3DTexture> _texture_list;
	TypedArray<Terrain3DMeshInstance> _mesh_list;

	GeneratedTexture _generated_albedo_textures;
	GeneratedTexture _generated_normal_textures;
	PackedColorArray _texture_colors;
	PackedFloat32Array _texture_uv_scales;
	PackedFloat32Array _texture_uv_rotations;

	void _swap_ids(ResType p_type, int p_old_id, int p_new_id);
	void _set_asset_list(ResType p_type, const TypedArray<Terrain3DAssetResource> &p_list);
	void _set_asset(ResType p_type, int p_index, const Ref<Terrain3DAssetResource> &p_asset);

	void _update_texture_files();
	void _update_texture_settings();

public:
	Terrain3DAssets();
	~Terrain3DAssets();

	void set_texture(int p_index, const Ref<Terrain3DTexture> &p_texture);
	Ref<Terrain3DTexture> get_texture(int p_index) const { return _texture_list[p_index]; }
	void set_texture_list(const TypedArray<Terrain3DTexture> &p_texture_list);
	TypedArray<Terrain3DTexture> get_texture_list() const { return _texture_list; }
	int get_texture_count() const { return _texture_list.size(); }
	RID get_albedo_array_rid() { return _generated_albedo_textures.get_rid(); }
	RID get_normal_array_rid() { return _generated_normal_textures.get_rid(); }
	PackedColorArray get_texture_colors() { return _texture_colors; }
	PackedFloat32Array get_texture_uv_scales() { return _texture_uv_scales; }
	PackedFloat32Array get_texture_uv_rotations() { return _texture_uv_rotations; }
	void update_texture_list();

	void set_mesh(int p_index, const Ref<Terrain3DMeshInstance> &p_mesh);
	Ref<Terrain3DMeshInstance> get_mesh(int p_index) const { return _mesh_list[p_index]; }
	void set_mesh_list(const TypedArray<Terrain3DMeshInstance> &p_mesh_list);
	TypedArray<Terrain3DMeshInstance> get_mesh_list() const { return _mesh_list; }
	int get_mesh_count() const { return _mesh_list.size(); }
	void update_mesh_list();

	void save();

protected:
	static void _bind_methods();
};

VARIANT_ENUM_CAST(Terrain3DAssets::ResType);

// Deprecated 0.9.1 - Remove Later
class Terrain3DTextureList : public Resource {
	GDCLASS(Terrain3DTextureList, Resource);
	CLASS_NAME();
	TypedArray<Terrain3DTexture> _textures;

public:
	Terrain3DTextureList() {}
	~Terrain3DTextureList() {}
	void set_textures(const TypedArray<Terrain3DTexture> &p_textures) { _textures = p_textures; }
	TypedArray<Terrain3DTexture> get_textures() const { return _textures; }

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("set_textures", "textures"), &Terrain3DTextureList::set_textures);
		ClassDB::bind_method(D_METHOD("get_textures"), &Terrain3DTextureList::get_textures);
		int ro_flags = PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_READ_ONLY;
		ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "textures", PROPERTY_HINT_ARRAY_TYPE, vformat("%tex_size/%tex_size:%tex_size", Variant::OBJECT, PROPERTY_HINT_RESOURCE_TYPE, "Terrain3DTextureList"), ro_flags), "set_textures", "get_textures");
	}
};
//////////////

#endif // TERRAIN3D_ASSETS_CLASS_H
