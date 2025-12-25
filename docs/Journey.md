# The Journey of lsfg-vk
Welcome to the journey of lsfg-vk, this is where I, the developer, PancakeTAS, share interesting stories about the development process and the challenges faced along the way.

---

## Porting LSFG to native Vulkan (Jul 6, 2025)
This document details the process of getting LSFG to work in a native Vulkan environment. It is not a guide. It is not a tutorial for doing this yourself. It simply describes the psychological torture I went through to make this project work

### Figuring out how LSFG works
The first thing I did was open Lossless Scaling in dnSpy, which quickly revealed that all the magic was in Lossless.dll file.\
Then I opened up Lossless.dll in IDA... and quickly found out that this is a D3D11 program.

I closed dnSpy and IDA. What this teaches you is that throwing IDA at something is not always the right move.

### Getting D3D11 to run on Linux
_"The first step to translating D3D11 to Vulkan, is to not translate D3D11 to Vulkan. Instead, it is to translate D3D11 to D3D11, and then to Vulkan."_ - Me (after not sleeping for 2 days)

It's true!

When you're trying to translate something that utilizes the GPU, it often comes in multiple parts:\
The **shaders** that run on the actual GPU and the **pipeline** used to coordinate the shaders

Like I said, I didn't translate D3D11 to Vulkan just yet. Instead I used what the Steam Deck, Proton, and native Linux titles from Valve use: DXVK.

DXVK is a project that pretends to be D3D11 and provides just enough methods to run most if not all Windows games written in D3D8 through D3D11 on Linux. It can also be used natively without proton/wine! And that's what I did.

#### Step 1: The shaders
The first step to translating everything to D3D11 was to get the shaders to work.
Vulkan uses SPIR-V and D3D11 uses DXBC. Thankfully DXVK can translate the shaders just fine.

I wrote a hook for Lossless Scaling that intercepted all D3D11 calls and dumped the shader files to my computer. Then I took those shader files and copied them over to Linux. I wrote a really small D3D11 app that loaded one shader, loaded a texture that I dumped into it, and dumped the outputs. Voila, WinMerge tells me the files are identical!

I did this to a few other shaders until I moved on to actually translating the pipeline.

#### Step 2: The pipeline
Translating the shader pipeline is the fun part of all of this. Attaching RenderDoc to LS on Windows is near impossible because it isn't a game by itself. Current RenderDoc simply does not detect anything and older versions crash immediately. NVIDIA Nsight Graphics also crashed when taking a capture.

The solution is a mix of IDA to get the dispatch math, self written C++ programs to dump the D3D11 commands, and a heck ton of WinMerge to compare my shader pipeline with the original.

Step by step I rewrote the shader pipeline in D3D11, just like Valve did with Portal 2, until I was done.
This process took a huuuge amount of time and there were many barricades in the way like when stuff just wouldn't work for no reason.

### Translating D3D11 to Vulkan
Having a working pipeline on Linux now means I can attach RenderDoc and get the exact Vulkan calls in a neatly organized manner.

This was my first time writing something in Vulkan, so I had to do quite a bit of research on Vulkan and read quite a bit of it. Originally my plan was to keep the DXVK pipeline and simply use Vulkan to hook the game. This plan was cut short however, because D3D11 does not have any synchronization primitives and Vulkan exclusively works with synchronization primitives. Since that didn't work, I had to translate the entire pipeline to Vulkan.

This process was fairly smooth and I did it in 27 hours. Twenty seven.. consecutive.. hours.. with no sleep.

_"Sometimes, no sleep is all you ne-"_ Okay I'm gonna stop with these stupid quotes, lol.

And there we have it, the Vulkan pipeline is completed. What follows is a bit of PE parsing to extract shaders from the exe at runtime, so that not everyone using the project needs to go through my painful process. And also a statically linked subset of DXVK is now used to translate the shaders without actually using DXVK.

### Credits
Huge thanks to [0xNULLderef](https://github.com/0xNULLderef) who sat through this with me. They were the first to suggest native DXVK and taught me how to use IDA and RenderDoc. This project would not have worked without them!

This document details the process of injecting frames into a Vulkan app. It describes the mechanism currently in use in lsfg-vk. This is not a guide, nor a tutorial. This is a technical document explaining what I did and how it works.

---

## Injecting into a Vulkan app (Jul 6, 2025)
This document details the process of injecting frames into a Vulkan app. It describes the mechanism currently in use in lsfg-vk. This is not a guide, nor a tutorial. This is a technical document explaining what I did and how it works.

### Injecting into a Vulkan app
There's plenty of ways to hook into a Vulkan app. At first this project used an `LD_PRELOAD` approach, where `dlopen`, `dlsym` and `dlclose` were hooked and replaced the Vulkan loader with it's own modded version.
Towards completion of the process I realized there was a better way to do this and seeing how it would fix a lot of issues I decided to implement it.

Enter: Vulkan layers.
When you link against Vulkan, statically or dynamically it does not matter, you're not linking against the actual library, but a loader. This Vulkan loader provides you with two functions called `vkDeviceGetProcAddr` and `vkInstanceGetProcAddr`. Calling these functions returns function pointers for each function requested.

This approach allows the Vulkan loader to return function pointers not to the actual driver, but an intermediate function. These intermediary functions are called layers and Vulkan stacks as many as you want ontop of each other.

lsfg-vk uses an implicit layer, which means it is implicitly enabled simply by existing in one of the many Vulkan layer folders, or by having an environment variable set. In our case, it loads when `ENABLE_LSFG` is set to 1.

### Hooking various functions
Each layer has its own `vkIntanceGetProcAddr` (and corresponding device-level) function. By default this function simply calls the _next_ layer's `vkInstanceGetProcAddr` function. If you wish to actually override a method, you simply store the next layer's function pointer and return your own method, which then eventually calls the next layer.

In order for us to inject frames into Vulkan apps, we need to add a few extensions to the Vulkan instance and each Vulkan device, as well as monitor created swapchains. We override `vkCreate{Instance,Device,SwapchainKHR}` as well as `vkDestroy{Instance,Device,SwapchainKHR}` and modify the creation info to add our own extensions, or in the case of the swapchain, add `TRANSFER_SRC` and `TRANSFER_DST` to the imageUsage flags (so that we can copy the to and from swapchain images).

### Grabbing and inserting frames into the app
This is where stuff gets interesting. First we hook `vkQueuePresentKHR`, which is the function used for presenting an image to the screen. Then we create a command buffer for copying the swapchain image in the parameters to some other image we created. That command buffer now takes the waitSemaphores from the present call and outputs its own semaphore, which is used in the original present call itself. Just like that, we've successfully grabbed a swapchain image. (I say "just like that", but this is several hundreds if not thousands of lines of code).

Inserting frames is where stuff gets interesting. If you just want to insert a frame, acquire an image from the swapchain using `vkAcquireNextImageKHR`, write or copy something to it, and call `vkQueuePresentKHR` a second time. The problem is synchronization.

### Synchronizing frame insertions
You don't know when the frame grab starts, because the waitSemaphores passed to you are GPU-only, you can not access them.
You don't know when your inserted frames are done rendering either, because that is also signaled by a semaphore on the GPU.
All of the code within `vkQueuePresentKHR` runs _immediately_, and you simply hook up one semaphore to another so that the GPU knows what to do. The GPU cant do math and go like "okay last frame took so long, so let's insert our frame once it's done rendering, plus an average time of 5 ms".

There is only two solutions to this problem. Solution one is to create a second thread and do a CPU-wait for rendering to finish, then sleep on the CPU for as long as you need to and insert the frame without a semaphore. The problem is that VkQueues are not thread safe and access needs to be synchronized on the CPU. This means you'll now be adding a mutex to each and every call using VkQueue, _and_ VkSwapchainKHR, _and_ the command buffer, etc.

The other solution is simply turning on the FIFO present mode. This mode queues up calls to vkQueuePresentKHR in order and takes a submitted frame each VBlank. This mode is more commonly known as V-Sync, but it's not exactly that. This solution only works because the amount of swapchain images is limited. Usually between 3 or 8 images can be prepared and presented (this does technically mean, that there may be up to 8 frames of latency if you do not handle this correctly.). Either way, changing the present mode is done in the swapchain creation, so we both switch to the FIFO present mode, as well as increase the amount of frames rendered by one plus the intermediate images. (Adding 1 here, because we're also delaying the rendering of one real frame).

### Integrating the frame generation
Integrating the frame generation into this is not really the target of this document, but I'll mention it anyways.

Frame generation runs in a separate Vulkan device, this is required because we need Vulkan 1.3 for DXVK's Shaders to run. We use the shared memory and shared semaphore extensions to get a file descriptor to image memory on the hook side and pass it to the frame gen side. Then we have two different VkImages on two devices which can both read and write to the same underlying memory.

From here it's a simple double buffering logic and sharing semaphores to synchronize completion and requests.

And there we have it, that's the entire application explained basically.
