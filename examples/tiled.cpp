#include "progression.h"
#include <chrono>

#define BLOCK_SIZE 16
using namespace Progression;

std::string rootDirectory;

class LightBallComponent : public Component {
public:
    LightBallComponent(GameObject* obj, GameObject* _ball) :
        Component(obj),
        ball(_ball)
    {
    }

    ~LightBallComponent() = default;
    void Start() {}
    void Stop() {}

    void Update() {
        gameObject->transform = ball->transform;
        gameObject->boundingBox.setCenter(gameObject->transform.position);
    }


    GameObject* ball;
};

class BounceComponent : public Component {
public:
    BounceComponent(GameObject* obj, const glm::vec3 startVel = glm::vec3(0, 0, 0)) :
        Component(obj),
        velocity(startVel)
    {
    }

    ~BounceComponent() = default;
    void Start() {}
    void Stop() {}

    void Update() {
        float dt = 1.0f / 30.0f;
        //float dt = Time::deltaTime();
        velocity.y += -9.81f * dt;
        gameObject->transform.position += velocity * dt;
        if (gameObject->transform.position.y < gameObject->transform.scale.x) {
            gameObject->transform.position.y = gameObject->transform.scale.x;
            velocity.y *= -.97;
        }
        gameObject->boundingBox.setCenter(gameObject->transform.position);
    }

    glm::vec3 velocity;
};

// argv[1] = path of the root directory
int main(int argc, char* argv[]) {
    srand(time(NULL));

	auto conf = PG::config::Config(PG_ROOT_DIR "configs/bouncing_ball.toml");
	if (!conf) {
		std::cout << "could not parse config file" << std::endl;
		exit(0);
	}

	PG::EngineInitialize(conf);
	auto scene = Scene::Load(PG_RESOURCE_DIR "scenes/bouncing_ball.pgscn");

	auto skybox = scene->getSkybox();

	auto camera = scene->GetCamera();
	camera->AddComponent<UserCameraComponent>(new UserCameraComponent(camera, 15));
	camera->SetRenderingPipeline(RenderingPipeline::TILED_DEFERRED);

	auto ballModel = ResourceManager::GetModel("metalBall");
	auto planeModel = ResourceManager::GetModel("metalFloor");

	int X, Z, startX, startZ;
	float DX, DZ;

	int numBalls;
	auto val = conf->get_table("app");
	numBalls = *val->get_as<int>("numBalls");

	if (numBalls == 9) {
		X = 3;
		Z = 3;
		startX = 0;
		startZ = 0;
		DX = 1;
		DZ = 1;
	}
	else if (numBalls == 100) {
		X = 10;
		Z = 10;
		startX = 0;
		startZ = 0;
		DX = 1;
		DZ = 1;
	} else if (numBalls == 400) {
		X = 20;
		Z = 20;
		startX = -X;
		startZ = -Z;
		DX = 1;
		DZ = 1;
	} else if (numBalls == 1000) {
		X = 32;
		Z = 32;
		startX = -X;
		startZ = -Z;
		DX = 1;
		DZ = 1;
	} else if (numBalls == 4000) {
		X = 64;
		Z = 64;
		startX = -X;
		startZ = -Z;
		DX = 1;
		DZ = 1;
	} else if (numBalls == 10000) {
		X = 100;
		Z = 100;
		startX = -X;
		startZ = -Z;
		DX = 1;
		DZ = 1;
	}

	float intensity = *val->get_as<float>("intensity");
	float cutOffIntensity = 0.03;
	for (float x = 0; x < X; x += DX) {
		for (float z = 0; z < Z; z += DZ) {
			float randHeight = 3 + 2 * (rand() / static_cast<float>(RAND_MAX));
			glm::vec3 pos = glm::vec3(startX + 2 * x, randHeight, startZ + 2 * z);
			GameObject* ballObj = new GameObject(Transform(pos, glm::vec3(0), glm::vec3(.5)));
			ballObj->AddComponent<ModelRenderer>(new ModelRenderer(ballObj, ballModel.get()));
			ballObj->AddComponent<BounceComponent>(new BounceComponent(ballObj));
			ballObj->boundingBox.Encompass(BoundingBox(glm::vec3(-1), glm::vec3(1)), ballObj->transform);

			glm::vec3 randColor = glm::vec3((rand() / static_cast<float>(RAND_MAX)), (rand() / static_cast<float>(RAND_MAX)), (rand() / static_cast<float>(RAND_MAX)));
			PG::Light* pl = new Light(PG::Light::Type::POINT, pos, randColor, intensity);
			pl->AddComponent<LightBallComponent>(new LightBallComponent(pl, ballObj));
			float lightRadius = std::sqrtf(intensity / cutOffIntensity);
			// lightRadius = 4;
			pl->boundingBox.Encompass(BoundingBox(glm::vec3(-1), glm::vec3(1)), Transform(pl->transform.position, glm::vec3(0), glm::vec3(lightRadius)));

			scene->AddGameObject(ballObj);
			scene->AddLight(pl);
		}
	}
	std::cout << "light Radius = " << std::sqrtf(intensity / cutOffIntensity) << std::endl;

	GameObject* planeObj = scene->GetGameObject("floor");
	planeObj->AddComponent<ModelRenderer>(new ModelRenderer(planeObj, planeModel.get()));

	// Note: After changing the input mode, should poll for events again
	Window::SetRelativeMouse(true);
	PG::Input::PollEvents();

	while (!PG::Input::GetKeyDown(PG::PG_K_P))
		PG::Input::PollEvents();

    // Game loop
    while (!PG::EngineShutdown) {
        PG::Window::StartFrame();
        PG::Input::PollEvents();

        if (PG::Input::GetKeyDown(PG::PG_K_ESC))
            PG::EngineShutdown = true;

        scene->Update();

		RenderSystem::Render(scene);
		/*
        // RenderSystem::UpdateLights(scene, camera);

        graphics::BindFrameBuffer(gBuffer);

        graphics::SetClearColor(glm::vec4(0));
        graphics::Clear();

        deferredShader.Enable();
        RenderSystem::UploadCameraProjection(deferredShader, *camera);

        const auto& gameObjects = scene->GetGameObjects();
        
        for (const auto& obj : scene->GetGameObjects()) {
            glm::mat4 M = obj->transform.GetModelMatrix();
            glm::mat4 MV = camera->GetV() * M;
            glm::mat4 normalMatrix = glm::transpose(glm::inverse(MV));
            glUniformMatrix4fv(deferredShader["modelViewMatrix"], 1, GL_FALSE, glm::value_ptr(MV));
            glUniformMatrix4fv(deferredShader["normalMatrix"], 1, GL_FALSE, glm::value_ptr(normalMatrix));
            for (const auto& mr : obj->GetComponent<ModelRenderer>()->meshRenderers) {
                glBindVertexArray(mr->vao);
                RenderSystem::UploadMaterial(deferredShader, *mr->material);
                glDrawElements(GL_TRIANGLES, mr->mesh->numTriangles * 3, GL_UNSIGNED_INT, 0);
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        graphics::Clear();

        // blit the deferred depth buffer to the main screen
        glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, Window::getWindowSize().x, Window::getWindowSize().y, 0, 0, Window::getWindowSize().x, Window::getWindowSize().y, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glm::mat4 VP = camera->GetP() * camera->GetV();
        glm::mat4 V = camera->GetV();
        const auto& pointLights = scene->GetPointLights();
        
        glm::vec4* lightBuffer = (glm::vec4*) glMapBufferRange( GL_SHADER_STORAGE_BUFFER, 0, 2 * sizeof(glm::vec4) * scene->GetNumPointLights(), bufMask);
        for (int i = 0; i < pointLights.size(); ++i) {
            float lightRadius = std::sqrtf(scene->GetPointLights()[i]->intensity / cutOffIntensity);
            //lightRadius = 6;
            lightBuffer[2 * i + 0] = V * glm::vec4(pointLights[i]->transform.position, 1);
            lightBuffer[2 * i + 0].w = lightRadius;
            lightBuffer[2 * i + 1] = glm::vec4(pointLights[i]->intensity * pointLights[i]->color, 1);
        }
        glUnmapBuffer( GL_SHADER_STORAGE_BUFFER );

        computeShader.Enable();
        glBindImageTexture(0, computeOutput, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
        glBindImageTexture(1, gPosition, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
        glBindImageTexture(2, gNormal, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
        glBindImageTexture(3, gDiffuse, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
        glBindImageTexture(4, gSpecularExp, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

        glUniform2i(computeShader["screenSize"], Window::getWindowSize().x, Window::getWindowSize().y);
        glUniform1i(computeShader["numPointLights"], scene->GetNumPointLights());
        glUniformMatrix4fv(computeShader["invProjMatrix"], 1, GL_FALSE, glm::value_ptr(glm::inverse(camera->GetP())));

        glDispatchCompute(Window::getWindowSize().x / BLOCK_SIZE, Window::getWindowSize().y / BLOCK_SIZE, 1);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        graphics::ToggleDepthBufferWriting(false);
        graphics::ToggleDepthTesting(false);

        combineShader.Enable();
        glBindVertexArray(quadVAO);
        graphics::Bind2DTexture(computeOutput, combineShader["computeOutput"], 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        graphics::ToggleDepthTesting(true);
        graphics::ToggleDepthBufferWriting(true);

        skybox->Render(*camera);
		*/

		skybox->Render(*camera);

		PG::Window::EndFrame();
    }

    PG::EngineQuit();

    return 0;
}
