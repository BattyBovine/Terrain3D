// Copyright © 2023 Cory Petkovsek, Roope Palmroos, and Contributors.

#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/resource_saver.hpp>

#include "logger.h"
#include "terrain_3d_assets.h"
#include "terrain_3d_util.h"

///////////////////////////
// Private Functions
///////////////////////////

void Terrain3DAssets::_swap_ids(AssetType p_type, int p_old_id, int p_new_id) {
	LOG(INFO, "Swapping asset id: ", p_old_id, " and id:", p_new_id);
	Array list;
	switch (p_type) {
		case TYPE_TEXTURE:
			list = _texture_list;
			break;
		case TYPE_MESH:
			list = _mesh_list;
			break;
		default:
			return;
	}

	if (p_old_id < 0 || p_old_id >= list.size()) {
		LOG(ERROR, "Old id out of range: ", p_old_id);
		return;
	}
	Ref<Terrain3DAssetResource> res_a = list[p_old_id];
	p_new_id = CLAMP(p_new_id, 0, list.size() - 1);
	if (p_new_id == p_old_id) {
		// Res_a new id was likely out of range, reset it
		res_a->_id = p_old_id;
		return;
	}

	Ref<Terrain3DAssetResource> res_b = list[p_new_id];
	res_a->_id = p_new_id;
	res_b->_id = p_old_id;
	list[p_new_id] = res_a;
	list[p_old_id] = res_b;

	switch (p_type) {
		case TYPE_TEXTURE:
			update_texture_list();
			break;
		case TYPE_MESH:
			emit_signal("meshes_changed");
			break;
		default:
			return;
	}
}

/**
 * _set_asset_list attempts to keep the asset id as saved in the resource file.
 * But if an ID is invalid or already taken, the new ID is changed to the next available one
 */
void Terrain3DAssets::_set_asset_list(AssetType p_type, const TypedArray<Terrain3DAssetResource> &p_list) {
	Array list;
	int max_size;
	switch (p_type) {
		case TYPE_TEXTURE:
			list = _texture_list;
			max_size = MAX_TEXTURES;
			break;
		case TYPE_MESH:
			list = _mesh_list;
			max_size = MAX_MESHES;
			break;
		default:
			return;
	}

	int array_size = CLAMP(p_list.size(), 0, max_size);
	list.resize(array_size);
	int filled_index = -1;
	// For all provided textures up to MAX SIZE
	for (int i = 0; i < array_size; i++) {
		Ref<Terrain3DAssetResource> res = p_list[i];
		int id = res->get_id();
		// If saved texture id is in range and doesn't exist, add it
		if (id >= 0 && id < array_size && !list[id]) {
			list[id] = res;
		} else {
			// Else texture id is invalid or slot is already taken, insert in next available
			for (int j = filled_index + 1; j < array_size; j++) {
				if (!list[j]) {
					res->set_id(j);
					list[j] = res;
					filled_index = j;
					break;
				}
			}
		}
		if (!res->is_connected("id_changed", callable_mp(this, &Terrain3DAssets::_swap_ids))) {
			LOG(DEBUG, "Connecting to id_changed");
			res->connect("id_changed", callable_mp(this, &Terrain3DAssets::_swap_ids));
		}
	}
}

void Terrain3DAssets::_set_asset(AssetType p_type, int p_index, const Ref<Terrain3DAssetResource> &p_asset) {
	Array list;
	int max_size;
	switch (p_type) {
		case TYPE_TEXTURE:
			list = _texture_list;
			max_size = MAX_TEXTURES;
			break;
		case TYPE_MESH:
			list = _mesh_list;
			max_size = MAX_MESHES;
			break;
		default:
			return;
	}

	if (p_index < 0 || p_index >= max_size) {
		LOG(ERROR, "Invalid asset index: ", p_index, " range is 0-", max_size);
		return;
	}
	// Delete asset if null
	if (p_asset.is_null()) {
		// If final asset, remove it
		if (p_index == list.size() - 1) {
			LOG(DEBUG, "Deleting asset id: ", p_index);
			list.pop_back();
		} else if (p_index < list.size()) {
			// Else just clear it
			Ref<Terrain3DAssetResource> res = list[p_index];
			res->clear();
			res->_id = p_index;
		}
	} else {
		// Else Insert/Add Asset at end if a high number
		if (p_index >= list.size()) {
			p_asset->_id = list.size();
			list.push_back(p_asset);
			if (!p_asset->is_connected("id_changed", callable_mp(this, &Terrain3DAssets::_swap_ids))) {
				LOG(DEBUG, "Connecting to id_changed");
				p_asset->connect("id_changed", callable_mp(this, &Terrain3DAssets::_swap_ids));
			}
		} else {
			// Else overwrite an existing slot
			list[p_index] = p_asset;
		}
	}
}

void Terrain3DAssets::_update_texture_files() {
	LOG(DEBUG, "Received texture_changed signal");
	_generated_albedo_textures.clear();
	_generated_normal_textures.clear();
	if (_texture_list.is_empty()) {
		return;
	}

	// Detect image sizes and formats

	LOG(INFO, "Validating texture sizes");
	Vector2i albedo_size = Vector2i(0, 0);
	Vector2i normal_size = Vector2i(0, 0);

	Image::Format albedo_format = Image::FORMAT_MAX;
	Image::Format normal_format = Image::FORMAT_MAX;
	bool albedo_mipmaps = true;
	bool normal_mipmaps = true;

	for (int i = 0; i < _texture_list.size(); i++) {
		Ref<Terrain3DTexture> texture_set = _texture_list[i];
		if (texture_set.is_null()) {
			continue;
		}
		Ref<Texture2D> albedo_tex = texture_set->_albedo_texture;
		Ref<Texture2D> normal_tex = texture_set->_normal_texture;

		// If this is the first texture, set expected size and format for the arrays
		if (albedo_tex.is_valid()) {
			Vector2i tex_size = albedo_tex->get_size();
			if (albedo_size.length() == 0.0) {
				albedo_size = tex_size;
			} else if (tex_size != albedo_size) {
				LOG(ERROR, "Texture ID ", i, " albedo size: ", tex_size, " doesn't match first texture: ", albedo_size);
				return;
			}
			Ref<Image> img = albedo_tex->get_image();
			Image::Format format = img->get_format();
			if (albedo_format == Image::FORMAT_MAX) {
				albedo_format = format;
				albedo_mipmaps = img->has_mipmaps();
			} else if (format != albedo_format) {
				LOG(ERROR, "Texture ID ", i, " albedo format: ", format, " doesn't match first texture: ", albedo_format);
				return;
			}
		}
		if (normal_tex.is_valid()) {
			Vector2i tex_size = normal_tex->get_size();
			if (normal_size.length() == 0.0) {
				normal_size = tex_size;
			} else if (tex_size != normal_size) {
				LOG(ERROR, "Texture ID ", i, " normal size: ", tex_size, " doesn't match first texture: ", normal_size);
				return;
			}
			Ref<Image> img = normal_tex->get_image();
			Image::Format format = img->get_format();
			if (normal_format == Image::FORMAT_MAX) {
				normal_format = format;
				normal_mipmaps = img->has_mipmaps();
			} else if (format != normal_format) {
				LOG(ERROR, "Texture ID ", i, " normal format: ", format, " doesn't match first texture: ", normal_format);
				return;
			}
		}
	}

	if (normal_size == Vector2i(0, 0)) {
		normal_size = albedo_size;
	} else if (albedo_size == Vector2i(0, 0)) {
		albedo_size = normal_size;
	}
	if (albedo_size == Vector2i(0, 0)) {
		albedo_size = Vector2i(1024, 1024);
		normal_size = Vector2i(1024, 1024);
	}

	// Generate TextureArrays and replace nulls with a empty image

	bool changed = false;
	if (_generated_albedo_textures.is_dirty() && albedo_size != Vector2i(0, 0)) {
		LOG(INFO, "Regenerating albedo texture array");
		Array albedo_texture_array;
		for (int i = 0; i < _texture_list.size(); i++) {
			Ref<Terrain3DTexture> texture_set = _texture_list[i];
			if (texture_set.is_null()) {
				continue;
			}
			Ref<Texture2D> tex = texture_set->_albedo_texture;
			Ref<Image> img;

			if (tex.is_null()) {
				img = Util::get_filled_image(albedo_size, COLOR_CHECKED, albedo_mipmaps, albedo_format);
				LOG(DEBUG, "ID ", i, " albedo texture is null. Creating a new one. Format: ", img->get_format());
				texture_set->_albedo_texture = ImageTexture::create_from_image(img);
			} else {
				img = tex->get_image();
				LOG(DEBUG, "ID ", i, " albedo texture is valid. Format: ", img->get_format());
			}
			albedo_texture_array.push_back(img);
		}
		if (!albedo_texture_array.is_empty()) {
			_generated_albedo_textures.create(albedo_texture_array);
			changed = true;
		}
	}

	if (_generated_normal_textures.is_dirty() && normal_size != Vector2i(0, 0)) {
		LOG(INFO, "Regenerating normal texture arrays");

		Array normal_texture_array;

		for (int i = 0; i < _texture_list.size(); i++) {
			Ref<Terrain3DTexture> texture_set = _texture_list[i];
			if (texture_set.is_null()) {
				continue;
			}
			Ref<Texture2D> tex = texture_set->_normal_texture;
			Ref<Image> img;

			if (tex.is_null()) {
				img = Util::get_filled_image(normal_size, COLOR_NORMAL, normal_mipmaps, normal_format);
				LOG(DEBUG, "ID ", i, " normal texture is null. Creating a new one. Format: ", img->get_format());
				texture_set->_normal_texture = ImageTexture::create_from_image(img);
			} else {
				img = tex->get_image();
				LOG(DEBUG, "ID ", i, " Normal texture is valid. Format: ", img->get_format());
			}
			normal_texture_array.push_back(img);
		}
		if (!normal_texture_array.is_empty()) {
			_generated_normal_textures.create(normal_texture_array);
			changed = true;
		}
	}

	if (changed) {
		emit_signal("textures_changed", Ref<Terrain3DAssets>(this));
	}
}

void Terrain3DAssets::_update_texture_settings() {
	LOG(DEBUG, "Received setting_changed signal");
	if (_texture_list.is_empty()) {
		return;
	}
	LOG(INFO, "Updating terrain color and scale arrays");
	_texture_colors.clear();
	_texture_uv_scales.clear();
	_texture_uv_rotations.clear();

	for (int i = 0; i < _texture_list.size(); i++) {
		Ref<Terrain3DTexture> texture_set = _texture_list[i];
		if (texture_set.is_null()) {
			continue;
		}
		_texture_colors.push_back(texture_set->get_albedo_color());
		_texture_uv_scales.push_back(texture_set->get_uv_scale());
		_texture_uv_rotations.push_back(texture_set->get_uv_rotation());
	}
	emit_signal("textures_changed", Ref<Terrain3DAssets>(this));
}

///////////////////////////
// Public Functions
///////////////////////////

Terrain3DAssets::Terrain3DAssets() {
}

Terrain3DAssets::~Terrain3DAssets() {
	_generated_albedo_textures.clear();
	_generated_normal_textures.clear();
}

void Terrain3DAssets::set_texture(int p_index, const Ref<Terrain3DTexture> &p_texture) {
	LOG(INFO, "Setting texture index: ", p_index);
	_set_asset(TYPE_TEXTURE, p_index, p_texture);
	update_texture_list();
}

void Terrain3DAssets::set_texture_list(const TypedArray<Terrain3DTexture> &p_texture_list) {
	LOG(INFO, "Setting texture list with ", p_texture_list.size(), " entries");
	_set_asset_list(TYPE_TEXTURE, p_texture_list);
	update_texture_list();
}

void Terrain3DAssets::update_texture_list() {
	LOG(INFO, "Reconnecting texture signals");
	for (int i = 0; i < _texture_list.size(); i++) {
		Ref<Terrain3DTexture> texture_set = _texture_list[i];

		if (texture_set.is_null()) {
			LOG(ERROR, "Texture at index ", i, " is null, but shouldn't be.");
			continue;
		}
		if (!texture_set->is_connected("file_changed", callable_mp(this, &Terrain3DAssets::_update_texture_files))) {
			LOG(DEBUG, "Connecting file_changed signal");
			texture_set->connect("file_changed", callable_mp(this, &Terrain3DAssets::_update_texture_files));
		}
		if (!texture_set->is_connected("setting_changed", callable_mp(this, &Terrain3DAssets::_update_texture_settings))) {
			LOG(DEBUG, "Connecting setting_changed signal");
			texture_set->connect("setting_changed", callable_mp(this, &Terrain3DAssets::_update_texture_settings));
		}
	}
	_generated_albedo_textures.clear();
	_generated_normal_textures.clear();
	_update_texture_files();
	_update_texture_settings();
}

void Terrain3DAssets::set_mesh(int p_index, const Ref<Terrain3DMeshInstance> &p_mesh) {
	LOG(INFO, "Setting mesh index: ", p_index);
	_set_asset(TYPE_MESH, p_index, p_mesh);
	update_mesh_list();
}

void Terrain3DAssets::set_mesh_list(const TypedArray<Terrain3DMeshInstance> &p_mesh_list) {
	LOG(INFO, "Setting mesh list with ", p_mesh_list.size(), " entries");
	_set_asset_list(TYPE_MESH, p_mesh_list);
	update_mesh_list();
}

void Terrain3DAssets::update_mesh_list() {
	LOG(INFO, "Reconnecting mesh instance signals");
	for (int i = 0; i < _mesh_list.size(); i++) {
		Ref<Terrain3DMeshInstance> mesh = _mesh_list[i];
		if (mesh.is_null()) {
			LOG(ERROR, "Mesh Instance at index ", i, " is null, but shouldn't be.");
			continue;
		}
		//if (!mesh->is_connected("file_changed", callable_mp(this, &Terrain3DAssets::_update_texture_files))) {
		//	LOG(DEBUG, "Connecting file_changed signal");
		//	mesh->connect("file_changed", callable_mp(this, &Terrain3DAssets::_update_texture_files));
		//}
		//if (!mesh->is_connected("setting_changed", callable_mp(this, &Terrain3DAssets::_update_mesh_settings))) {
		//	LOG(DEBUG, "Connecting setting_changed signal");
		//	mesh->connect("setting_changed", callable_mp(this, &Terrain3DAssets::_update_mesh_settings));
		//}
	}
	emit_signal("meshes_changed");
}

void Terrain3DAssets::save() {
	String path = get_path();
	if (path.get_extension() == "tres" || path.get_extension() == "res") {
		LOG(DEBUG, "Attempting to save texture list to external file: " + path);
		Error err;
		err = ResourceSaver::get_singleton()->save(this, path, ResourceSaver::FLAG_COMPRESS);
		ERR_FAIL_COND(err);
		LOG(DEBUG, "ResourceSaver return error (0 is OK): ", err);
		LOG(INFO, "Finished saving texture list");
	}
}

///////////////////////////
// Protected Functions
///////////////////////////

void Terrain3DAssets::_bind_methods() {
	BIND_ENUM_CONSTANT(TYPE_TEXTURE);
	BIND_ENUM_CONSTANT(TYPE_MESH);
	BIND_CONSTANT(MAX_TEXTURES);
	BIND_CONSTANT(MAX_MESHES);

	ClassDB::bind_method(D_METHOD("set_mesh", "index", "mesh"), &Terrain3DAssets::set_mesh);
	ClassDB::bind_method(D_METHOD("get_mesh", "index"), &Terrain3DAssets::get_mesh);
	ClassDB::bind_method(D_METHOD("set_mesh_list", "mesh_list"), &Terrain3DAssets::set_mesh_list);
	ClassDB::bind_method(D_METHOD("get_mesh_list"), &Terrain3DAssets::get_mesh_list);
	ClassDB::bind_method(D_METHOD("get_mesh_count"), &Terrain3DAssets::get_mesh_count);

	ClassDB::bind_method(D_METHOD("set_texture", "index", "texture"), &Terrain3DAssets::set_texture);
	ClassDB::bind_method(D_METHOD("get_texture", "index"), &Terrain3DAssets::get_texture);
	ClassDB::bind_method(D_METHOD("set_texture_list", "texture_list"), &Terrain3DAssets::set_texture_list);
	ClassDB::bind_method(D_METHOD("get_texture_list"), &Terrain3DAssets::get_texture_list);
	ClassDB::bind_method(D_METHOD("get_texture_count"), &Terrain3DAssets::get_texture_count);

	ClassDB::bind_method(D_METHOD("save"), &Terrain3DAssets::save);

	int ro_flags = PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_READ_ONLY;
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "mesh_list", PROPERTY_HINT_ARRAY_TYPE, vformat("%tex_size/%tex_size:%tex_size", Variant::OBJECT, PROPERTY_HINT_RESOURCE_TYPE, "Terrain3DMeshInstance"), ro_flags), "set_mesh_list", "get_mesh_list");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "texture_list", PROPERTY_HINT_ARRAY_TYPE, vformat("%tex_size/%tex_size:%tex_size", Variant::OBJECT, PROPERTY_HINT_RESOURCE_TYPE, "Terrain3DTexture"), ro_flags), "set_texture_list", "get_texture_list");

	ADD_SIGNAL(MethodInfo("meshes_changed"));
	ADD_SIGNAL(MethodInfo("textures_changed"));
}
