LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../SDL
SDL_MIXER_PATH := ../SDL_mixer

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include \
                    $(LOCAL_PATH)/$(SDL_MIXER_PATH)/include

LOCAL_SRC_FILES := main.c

LOCAL_SHARED_LIBRARIES := SDL2 SDL2_mixer

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -lOpenSLES -llog -landroid

include $(BUILD_SHARED_LIBRARY)
