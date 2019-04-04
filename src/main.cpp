#ifdef __EMSCRIPTEN__
	#include <emscripten/emscripten.h>
	#include <functional>

	static void dispatch_main(void* fp)
	{
		std::function<void()>* func = (std::function<void()>*)fp;
		(*func)();
	}
#endif

#include <SDL.h>


int main(int argc, char* args[])
{
	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Running...\n");
	SDL_Window* window = nullptr;
	SDL_Surface* surface = nullptr;

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
	else
	{
		if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"))
		{
			SDL_Log("Warning: Linear texture filtering not enabled!");
		}
#ifdef __ANDROID__
		SDL_DisplayMode displayMode;
		if (SDL_GetCurrentDisplayMode(0, &displayMode) == 0)		
			window = SDL_CreateWindow("SDL Framework", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, displayMode.w, displayMode.h, SDL_WINDOW_SHOWN);
		else
			SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Could not get display mode for video display #%d: %s", 0, SDL_GetError());
#else
		window = SDL_CreateWindow("SDL Framework", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024, 576, SDL_WINDOW_SHOWN);
#endif
		if (window == nullptr)
			SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Window could not be created! SDL_Error: %s\n", SDL_GetError());
		else
		{
            SDL_Renderer* gRenderer = SDL_CreateRenderer( window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );
            if( gRenderer == NULL )
                SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Renderer could not be created! SDL Error: %s\n", SDL_GetError() );
			else      
				SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0x00, 0xFF);

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
#ifdef __ANDROID__
					if (e.type == SDL_FINGERDOWN)
#else
					if (e.type == SDL_MOUSEBUTTONDOWN)					
						if (e.button.button == SDL_BUTTON_LEFT)
#endif						
							SDL_SetRenderDrawColor(gRenderer, 0xFF, 0x00, 0xFF, 0xFF);
#ifdef __ANDROID__
					if (e.type == SDL_FINGERUP)
#else
					if (e.type == SDL_MOUSEBUTTONUP)
						if (e.button.button == SDL_BUTTON_LEFT)
#endif						
							SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0x00, 0xFF);
				}
				
				SDL_RenderClear(gRenderer);
				SDL_RenderPresent(gRenderer);

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