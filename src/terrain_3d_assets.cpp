// Copyright © 2023 Cory Petkovsek, Roope Palmroos, and Contributors.

#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/resource_saver.hpp>

#include "logger.h"
#include "terrain_3d_assets.h"
#include "terrain_3d_util.h"

///////////////////////////
// Private Functions
///////////////////////////

void Terrain3DAssets::_swap_textures(ResType p_type, int p_old_id, int p_new_id) {
	LOG(MESG, "Swapping textures id: ", p_old_id, " and id:", p_new_id);
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
	LOG(WARN, "res id: ", res_a->_id);
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
			break;
		default:
			return;
	}

	//Ref<Terrain3DTexture> texture_a = _texture_list[p_old_id];
	//p_new_id = CLAMP(p_new_id, 0, _texture_list.size() - 1);
	//if (p_new_id == p_old_id) {
	//	// Texture_a new id was likely out of range, reset it
	//	texture_a->_id = p_old_id;
	//	return;
	//}

	//LOG(DEBUG, "Swapping textures id: ", p_old_id, " and id:", p_new_id);
	//Ref<Terrain3DTexture> texture_b = _texture_list[p_new_id];
	//texture_a->_id = p_new_id;
	//texture_b->_id = p_old_id;
	//_texture_list[p_new_id] = texture_a;
	//_texture_list[p_old_id] = texture_b;

	//update_texture_list();
}

void Terrain3DAssets::_update_texture_files() {
	//LOG(DEBUG, "Received texture_changed signal");
	LOG(MESG, "Received texture_changed signal");
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
		Ref<Texture2D> albedo_tex = texture_set->get_albedo_texture();
		Ref<Texture2D> normal_tex = texture_set->get_normal_texture();

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

void Terrain3DAssets::update_lists() {
	update_mesh_list();
	update_texture_list();
}

void Terrain3DAssets::update_mesh_list() {
	LOG(INFO, "Reconnecting mesh instance signals");
	for (int i = 0; i < _mesh_list.size(); i++) {
		Ref<Terrain3DMeshInstance> mesh = _mesh_list[i];

		if (mesh.is_null()) {
			LOG(ERROR, "Mesh Instance at index ", i, " is null, but shouldn't be.");
			continue;
		}
		if (!mesh->is_connected("file_changed", callable_mp(this, &Terrain3DAssets::_update_texture_files))) {
			LOG(DEBUG, "Connecting file_changed signal");
			mesh->connect("file_changed", callable_mp(this, &Terrain3DAssets::_update_texture_files));
		}
		//if (!mesh->is_connected("setting_changed", callable_mp(this, &Terrain3DAssets::_update_mesh_settings))) {
		//	LOG(DEBUG, "Connecting setting_changed signal");
		//	mesh->connect("setting_changed", callable_mp(this, &Terrain3DAssets::_update_mesh_settings));
		//}
	}
}

void Terrain3DAssets::set_mesh(int p_index, const Ref<Terrain3DMeshInstance> &p_mesh) {
	LOG(INFO, "Setting mesh index: ", p_index);
	if (p_index < 0 || p_index >= MAX_MESHES) {
		LOG(ERROR, "Invalid mesh index: ", p_index, " range is 0-", MAX_MESHES);
		return;
	}
	//Delete mesh
	if (p_mesh.is_null()) {
		// If final mesh, remove it
		if (p_index == get_mesh_count() - 1) {
			LOG(DEBUG, "Deleting mesh id: ", p_index);
			_mesh_list.pop_back();
		} else if (p_index < get_mesh_count()) {
			// Else just clear it
			Ref<Terrain3DMeshInstance> mesh = _mesh_list[p_index];
			mesh->clear();
			mesh->_id = p_index;
		}
	} else {
		// Else Insert/Add mesh
		// At end if a high number
		if (p_index >= get_mesh_count()) {
			p_mesh->_id = get_mesh_count();
			_mesh_list.push_back(p_mesh);
			if (!p_mesh->is_connected("id_changed", callable_mp(this, &Terrain3DAssets::_swap_textures))) {
				LOG(DEBUG, "Connecting to id_changed");
				p_mesh->connect("id_changed", callable_mp(this, &Terrain3DAssets::_swap_textures));
			}
		} else {
			// Else overwrite an existing slot
			_mesh_list[p_index] = p_mesh;
		}
	}
	update_mesh_list();
}

void Terrain3DAssets::set_mesh_list(const TypedArray<Terrain3DMeshInstance> &p_mesh_list) {
	LOG(INFO, "Setting mesh list");
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

void Terrain3DAssets::set_texture(int p_index, const Ref<Terrain3DTexture> &p_texture) {
	LOG(INFO, "Setting texture index: ", p_index);
	if (p_index < 0 || p_index >= MAX_TEXTURES) {
		LOG(ERROR, "Invalid texture index: ", p_index, " range is 0-", MAX_TEXTURES);
		return;
	}
	//Delete texture
	if (p_texture.is_null()) {
		// If final texture, remove it
		if (p_index == get_texture_count() - 1) {
			LOG(DEBUG, "Deleting texture id: ", p_index);
			_texture_list.pop_back();
		} else if (p_index < get_texture_count()) {
			// Else just clear it
			Ref<Terrain3DTexture> texture = _texture_list[p_index];
			texture->clear();
			texture->_id = p_index;
		}
	} else {
		// Else Insert/Add Texture
		// At end if a high number
		if (p_index >= get_texture_count()) {
			p_texture->_id = get_texture_count();
			_texture_list.push_back(p_texture);
			if (!p_texture->is_connected("id_changed", callable_mp(this, &Terrain3DAssets::_swap_textures))) {
				LOG(DEBUG, "Connecting to id_changed");
				p_texture->connect("id_changed", callable_mp(this, &Terrain3DAssets::_swap_textures));
			}
		} else {
			// Else overwrite an existing slot
			_texture_list[p_index] = p_texture;
		}
	}
	update_texture_list();
}

/**
 * set_textures attempts to keep the texture_id as saved in the resource file.
 * But if an ID is invalid or already taken, the new ID is changed to the next available one
 */
void Terrain3DAssets::set_texture_list(const TypedArray<Terrain3DTexture> &p_texture_list) {
	LOG(INFO, "Setting texture list");
	int max_size = CLAMP(p_texture_list.size(), 0, MAX_TEXTURES);
	_texture_list.resize(max_size);
	int filled_index = -1;
	// For all provided textures up to MAX SIZE
	for (int i = 0; i < max_size; i++) {
		Ref<Terrain3DTexture> texture = p_texture_list[i];
		int id = texture->get_id();
		// If saved texture id is in range and doesn't exist, add it
		if (id >= 0 && id < max_size && !_texture_list[id]) {
			_texture_list[id] = texture;
		} else {
			// Else texture id is invalid or slot is already taken, insert in next available
			for (int j = filled_index + 1; j < max_size; j++) {
				if (!_texture_list[j]) {
					texture->set_id(j);
					_texture_list[j] = texture;
					filled_index = j;
					break;
				}
			}
		}
		if (!texture->is_connected("id_changed", callable_mp(this, &Terrain3DAssets::_swap_textures))) {
			LOG(DEBUG, "Connecting to id_changed");
			texture->connect("id_changed", callable_mp(this, &Terrain3DAssets::_swap_textures));
		}
	}
	update_texture_list();
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
	BIND_CONSTANT(MAX_TEXTURES);

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
