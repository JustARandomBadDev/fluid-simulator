#include "core/camera.hpp"

#include <algorithm>
#include <cmath>

namespace fluid::core {

Camera::Camera(
    glm::vec3 initialPosition,
    float initialFov,
    float aspectRatio,
    float initialNearPlane,
    float initialFarPlane
)
    : position(initialPosition),
      yaw(-90.0f),
      pitch(0.0f),
      fov(initialFov),
      nearPlane(initialNearPlane),
      farPlane(initialFarPlane) {

    worldUp = glm::vec3(0.0f, -1.0f, 0.0f);
    front = glm::vec3(0.0f, 0.0f, -1.0f);

    projectionMatrix = glm::perspective(glm::radians(initialFov), aspectRatio, initialNearPlane, initialFarPlane);
    updateViewMatrix();
}

void Camera::translate(glm::vec3 delta) {
    position += delta;
    updateViewMatrix();
}

void Camera::setPosition(glm::vec3 newPosition) {
    position = newPosition;
    updateViewMatrix();
}

void Camera::rotate(float yawOffset, float pitchOffset) {
    yaw = std::fmod(yaw + yawOffset, 360.0f);
    pitch += pitchOffset;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    updateViewMatrix();
}

void Camera::setRotation(float newYaw, float newPitch) {
    yaw = std::fmod(newYaw, 360.0f);
    pitch = std::clamp(newPitch, -89.0f, 89.0f);
    updateViewMatrix();
}

void Camera::updateViewMatrix() {
    front = glm::normalize(glm::vec3(
        cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
        sin(glm::radians(pitch)),
        sin(glm::radians(yaw)) * cos(glm::radians(pitch))
    ));

    viewMatrix = glm::lookAt(position, position + front, worldUp);
}

void Camera::updateProjection(float aspectRatio) {
    projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
}

} // namespace fluid::core
