#include "Player.hpp"
#include "DebugSystem.hpp"

Player::Player(const InputManager& inputManager, TextureManager& textureManager, Renderer& renderer)
	: IEntity(textureManager, renderer),
	inputManager(inputManager),
	renderer(renderer)
{
	animations.resize(11);
	spriteSize = 56;
	texture = textureManager.GetTexture("assets/player/char_blue_1.png");
	collisionBox = { 19, 25, 21, 32 };
	animations.at(ANIM_IDLE) = Animation(6, 0.6f);
	animations.at(ANIM_WALK) = Animation(8, 0.8f);
	animations.at(ANIM_JUMP) = Animation(8, 0.4f, false);
	animations.at(ANIM_FALL) = Animation(8, 0.4f, false);


	currentAnimation = ANIM_IDLE;
	dynamic = true;

	accelerationRate = 2000;
	jumpForce = -300;
	maxSpeedX = 300;

}


void Player::Update(const float deltaTime, const float stepSize)
{
	AnimatedGameObject::Update(stepSize, stepSize);
	ReadInput();

	switch (state) {
	case IDLE:
		if (direction) {
			state = WALKING;
		}

		if (jumpPressed && grounded) {
			state = JUMPING;
		}
		break;
	case WALKING:
		if (!direction) {
			state = IDLE;
		}
		if (jumpPressed && grounded) {
			state = JUMPING;
		}
		break;
	case JUMPING:
		if (grounded) {
			if (direction) {
				state = WALKING;
			}
			else {
				state = IDLE;
			}

		}
		else if (velocity.y > 0) {
			state = FALLING;
		}
		break;
	case FALLING:
		if (grounded) {
			if (direction) {
				state = WALKING;
			}
			else {
				state = IDLE;
			}
		}
		break;
	}

	UpdateMovement(stepSize);
	UpdateAnimation();

	if (DebugSystem::Instance().player.showState) {
		DebugSystem::Instance().debugMessages.push_back("Player State: " + std::to_string(state));
	}
}

void Player::UpdateAnimation()
{
	switch (state) {
	case IDLE: currentAnimation = ANIM_IDLE; break;
	case WALKING: currentAnimation = ANIM_WALK; break;
	case JUMPING:
		if (currentAnimation != ANIM_JUMP) {
			animations.at(ANIM_JUMP).start();
			currentAnimation = ANIM_JUMP;
		}
		break;
	case FALLING:
		if (currentAnimation != ANIM_FALL) {
			animations.at(ANIM_FALL).start();
			currentAnimation = ANIM_FALL;
		}
	}
}

void Player::ReadInput()
{
	float horizontalDirection = 0;
	if (inputManager.IsKeyDown(Key::Left)) {
		horizontalDirection += -1;
	}
	if (inputManager.IsKeyDown(Key::Right)) {
		horizontalDirection += 1;
	}

	direction = horizontalDirection;

	jumpPressed = inputManager.IsKeyPressed(Key::Up);
}

void Player::UpdateMovement(const float stepSize)
{
	if (grounded && state == JUMPING) {
		SDL_Log("jumping! %s", grounded ? "grounded" : "not grounded");
		velocity.y = jumpForce;
		grounded = false;
	}

	MoveHorizontal(direction, stepSize);
}

void Player::MoveHorizontal(const float currentDirection, const float stepSize) {
	if (currentDirection) {

		if ((velocity.x > 0 && currentDirection < 0) ||
			(velocity.x < 0 && currentDirection > 0))
		{
			// Directional change, apply stronger acceleration
			acceleration.x = currentDirection * accelerationRate * 7.5f;
		}
		else {
			acceleration.x = currentDirection * accelerationRate;
		}
	}
	else {
		// decelerate
		if (velocity.x) {
			const float factor = velocity.x > 0 ? -2.5f : 2.5f;
			const float amount = factor * accelerationRate * stepSize;
			if (std::abs(velocity.x) < std::abs(amount)) {
				acceleration.x = 0;
				velocity.x = 0;
			}
			else {
				acceleration.x += amount;
				velocity.x += amount;
			}
		}
	}
}

void Player::SetGrounded(bool grounded)
{
	this->grounded = grounded;
}


void Player::Render()
{
	renderer.DrawAnimatedGameObject(*this);
}
