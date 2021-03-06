#include "Entity.h"
#include "Animation.h"
#include "j1Render.h"
#include "j1App.h"
#include "p2Defs.h"
#include "j1CollisionManager.h"
#include "p2Log.h"
#include "j1App.h"
#include "j1Textures.h"

Entity::Entity() :
	sprite(nullptr),
	currentAnim(nullptr),
	actionState(IDLE),
	currentDir(D_DOWN),
	lastPos(iPoint()),
	currentPos(lastPos),
	col(nullptr),
	colPivot(iPoint()),
	type(ENTITY_TYPE(0)),
	life(1),
	damaged(false),
	toDelete(false)
{
	anim.clear();
}

Entity::Entity(ENTITY_TYPE type) :
	sprite(nullptr),
	currentAnim(nullptr),
	actionState(IDLE),
	currentDir(D_DOWN),
	lastPos(iPoint()),
	currentPos(iPoint()),
	col(nullptr),
	colPivot(iPoint()),
	type(type),
	life(1)
{
	anim.clear();
}

Entity::~Entity()
{
	RELEASE(sprite);
	RELEASE(currentAnim);

	col->to_delete = true;
	col = nullptr;

	anim.clear();
}

bool Entity::Draw()
{
	bool ret = true;

	currentAnim = &anim.find({ actionState, currentDir })->second;
	sprite->updateSprite(currentPos, currentAnim->pivot, currentAnim->getCurrentFrame(), currentAnim->flip);
	App->render->Draw(sprite);

	return ret;
}

bool Entity::LoadAttributes(pugi::xml_node attributes)
{
	bool ret = true;

	// base stats
	pugi::xml_node node = attributes.child("base");
	life = node.attribute("life").as_int(1);
	speed = node.attribute("speed").as_int(1);
	

	//collider
	node = attributes.child("collider");
	colPivot = { node.attribute("x").as_int(8), node.attribute("y").as_int(12) };
	col = App->collisions->AddCollider({ currentPos.x, currentPos.y, node.attribute("w").as_int(16), node.attribute("h").as_int(15) }, COLLIDER_TYPE(node.attribute("type").as_int(-1)), ((j1Module*)App->game));
	col->parent = this;
	node = attributes.child("animation");

	//TODO: load three links animation properly
	if (ret = LoadAnimations(node.attribute("file").as_string()))
	{
		node = attributes.child("texture");
		std::string texFile = node.attribute("file").as_string();

		SDL_Texture* tex = App->tex->Load(texFile.c_str());

		if (ret = (tex != NULL))
		{
			currentAnim = &anim.find({ actionState, currentDir })->second;
			sprite = new Sprite(tex, currentPos, SCENE, currentAnim->getCurrentFrame(), currentAnim->pivot);
		}
	}

	return ret;
}

bool Entity::LoadAnimations(std::string file)
{
	bool ret = true;

	pugi::xml_document	anim_file;
	pugi::xml_node		ent;
	char* buff;
	int size = App->fs->Load(file.c_str(), &buff);
	pugi::xml_parse_result result = anim_file.load_buffer(buff, size);
	RELEASE(buff);

	if (ret = (result != NULL))
	{
		ent = anim_file.child("animations");

		for (pugi::xml_node action = ent.child("IDLE"); action != NULL; action = action.next_sibling())
		{
			for (pugi::xml_node dir = action.child("UP"); dir != action.child("loop"); dir = dir.next_sibling())
			{
				std::pair<ACTION_STATE, DIRECTION> p;
				p.first = ACTION_STATE(action.child("name").attribute("value").as_int());
				p.second = DIRECTION(dir.first_child().attribute("name").as_int());

				Animation anims;
				anims.setAnimation(
					dir.first_child().attribute("x").as_int(),
					dir.first_child().attribute("y").as_int(),
					dir.first_child().attribute("w").as_int(),
					dir.first_child().attribute("h").as_int(),
					dir.first_child().attribute("frameNumber").as_int(),
					dir.first_child().attribute("margin").as_int());

				anims.loop = action.child("loop").attribute("value").as_bool();
				anims.speed = action.child("speed").attribute("value").as_float();
				anims.pivot.x = dir.first_child().attribute("pivot_x").as_int();
				anims.pivot.y = dir.first_child().attribute("pivot_y").as_int();
				anims.flip = SDL_RendererFlip(dir.first_child().attribute("flip").as_int());

				anim.insert(std::pair<std::pair<ACTION_STATE, DIRECTION>, Animation >(p, anims));
			}
		}
	}
	else
	{
		LOG("Could not load attributes xml file. Pugi error: %s", result.description());
	}

	return ret;
}

//Displace the entity a given X and Y taking in account collisions w/map
void Entity::Move(int x, int y)
{
	currentPos.x += x;
	UpdateCollider();
}

void Entity::UpdateCollider()
{
	col->rect.x = currentPos.x - colPivot.x;
	col->rect.y = currentPos.y - colPivot.y;
}