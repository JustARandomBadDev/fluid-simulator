#ifndef FLUID_CORE_CAMERA_HPP
#define FLUID_CORE_CAMERA_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace fluid::core {

class Camera {
  public:
    Camera(
        glm::vec3 position = {0.0f, 0.0f, 0.0f},
        float fov = 70.0f,
        float aspectRatio = 1.0f,
        float nearPlane = 0.1f,
        float farPlane = 1000.0f
    );

    void translate(glm::vec3 delta);
    void setPosition(glm::vec3 position);
    void rotate(float yawOffset, float pitchOffset);
    void setRotation(float yaw, float pitch);
    void updateProjection(float aspectRatio);

    glm::mat4 getViewMatrix() const {
        return viewMatrix;
    }
    glm::mat4 getProjectionMatrix() const {
        return projectionMatrix;
    }
    glm::vec3 getPosition() const {
        return position;
    }
    glm::vec3 getDirection() const {
        return front;
    }
    glm::vec3 getWorldUp() const {
        return worldUp;
    }

  private:
    void updateViewMatrix();

    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 worldUp;

    float yaw;
    float pitch;
    float fov;
    float nearPlane;
    float farPlane;

    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
};

} // namespace fluid::core

#endif
