#ifdef __EMSCRIPTEN__
	#include <emscripten/emscripten.h>
	#include <functional>

	static void dispatch_main(void* fp)
	{
		std::function<void()>* func = (std::function<void()>*)fp;
		(*func)();
	}
#endif

#include <SDL2/SDL.h>

int main(int argc, char* args[])
{
	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Running...\n");
	SDL_Window* window = nullptr;
	SDL_Surface* surface = nullptr;

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
	else
	{
		window = SDL_CreateWindow("SDL Framework", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_SHOWN);
		if (window == nullptr)
			SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Window could not be created! SDL_Error: %s\n", SDL_GetError());
		else
		{
			surface = SDL_GetWindowSurface(window);
			SDL_FillRect(surface, nullptr, SDL_MapRGB(surface->format, 0xFF, 0x00, 0xFF));

			SDL_UpdateWindowSurface(window);

			bool quit = false;

			SDL_Event e;
#ifdef __EMSCRIPTEN__
			std::function<void()> mainLoop = [&]() {
#else
			while (!quit) {
#endif
				while (SDL_PollEvent(&e) != 0)
				{
					if (e.type == SDL_QUIT)
						quit = true;
				}
			}
#ifdef __EMSCRIPTEN__
			; emscripten_set_main_loop_arg(dispatch_main, &mainLoop, 0, 1);
#endif
		}
	}

	SDL_DestroyWindow(window);
	SDL_Quit();

	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Exiting...\n");
	return 0;
}