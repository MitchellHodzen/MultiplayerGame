#include "ecdb.h"
#include <clay.h>
#include "game_state.h"
#include "ring_buffer.h"
#include "chat_buffers.h"

static int previousChatBottom = 0;

inline static Clay_RenderCommandArray Build_UI(struct Game_Data* game_data, bool chatting, bool client_side_prediction_enabled, bool client_side_interpolation_enabled, float delta_time)
{
    Clay_BeginLayout();
    CLAY(CLAY_ID("WholeScreen"), { .layout = { .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) }, .padding = {.left = 0, .right = 0, .top = 0, .bottom = 0 } , .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP}, .layoutDirection = CLAY_TOP_TO_BOTTOM}, .backgroundColor = {0,0,0,0} }) {
        CLAY(CLAY_ID("UI_Top_Half"), { .layout = { .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_PERCENT(0.5f) }, .padding = {.left = 0, .right = 0, .top = 0, .bottom = 0 } , .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP}, .layoutDirection = CLAY_TOP_TO_BOTTOM}, .backgroundColor = {0,0,0,0} }) {
            CLAY(CLAY_ID("Controls_Text"), {
                .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM, .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) }, .childGap = 1, .padding = CLAY_PADDING_ALL(5), .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP} }, .backgroundColor = { 0, 0, 0, 0 }
            }) {
                CLAY_TEXT(CLAY_STRING("Controls:"), { .fontSize = 24, .textColor = {255, 255, 255, 150} });
                CLAY_TEXT(CLAY_STRING("WASD: Move"), { .fontSize = 24, .textColor = {255, 255, 255, 150} });
                CLAY(CLAY_ID("Prediction_Instructions"), { .layout = { .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) }, .layoutDirection = CLAY_LEFT_TO_RIGHT, .childGap = 3, .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER } }, .backgroundColor = {0,0,0,0} }) {
                if (client_side_prediction_enabled)
                    {
                        CLAY_TEXT(CLAY_STRING("1: Toggle Prediction (enabled)"), { .fontSize = 24, .textColor = {255, 255, 255, 150} });
                    }
                    else
                    {
                        CLAY_TEXT(CLAY_STRING("1: Toggle Prediction "), { .fontSize = 24, .textColor = {255, 255, 255, 150} });
                        CLAY_TEXT(CLAY_STRING("(disabled)"), { .fontSize = 24, .textColor = {255, 0, 0, 255} });
                    }
                }

                CLAY(CLAY_ID("Interpolation_Instructions"), { .layout = { .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) }, .layoutDirection = CLAY_LEFT_TO_RIGHT, .childGap = 3, .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER } }, .backgroundColor = {0,0,0,0} }) {
                if (client_side_interpolation_enabled)
                    {
                        CLAY_TEXT(CLAY_STRING("2: Toggle Interpolation (enabled)"), { .fontSize = 24, .textColor = {255, 255, 255, 150} });
                    }
                    else
                    {
                        CLAY_TEXT(CLAY_STRING("2: Toggle Interpolation "), { .fontSize = 24, .textColor = {255, 255, 255, 150} });
                        CLAY_TEXT(CLAY_STRING("(disabled)"), { .fontSize = 24, .textColor = {255, 0, 0, 255} });
                    }
                }
            }
        }
        CLAY(CLAY_ID("UI_Bottom_Half"), { .layout = { .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_PERCENT(0.5f) }, .padding = {.left = 0, .right = 0, .top = 0, .bottom = 0 } , .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP}, .layoutDirection = CLAY_TOP_TO_BOTTOM}, .backgroundColor = {0,0,0,0} }) {
            CLAY(CLAY_ID("ChatParentContainer"), { .layout = { .sizing = { .width = CLAY_SIZING_PERCENT(0.5f), .height = CLAY_SIZING_GROW(0) }, .padding = {.left = 5, .right = 0, .top = 0, .bottom = 5 } , .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_BOTTOM}, .layoutDirection = CLAY_TOP_TO_BOTTOM}, .backgroundColor = {0,0,0,0} }) {
                CLAY(CLAY_ID("FullChatWindowContainer"), {
                    .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM, .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) }, .childGap = 3, .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_BOTTOM} }, .backgroundColor = { 50, 50, 50, 100 }
                }) {
                    CLAY(CLAY_ID("ChatHistoryContainer"), {
                    .clip = { .vertical = true, .childOffset = { Clay_GetScrollOffset().x, Clay_GetScrollOffset().y } }, .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM, .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) }, .childGap = 0, .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_BOTTOM} }
                    }) {
                        for (unsigned int i = 0; i < game_data->chat_buffers->chat_history_buffer->buffer_size; ++i)
                        {
                            CLAY(CLAY_IDI("Chat", i), { .layout = { .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) }, .padding = CLAY_PADDING_ALL(5), .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER } } }) {
                                CLAY_TEXT(((Clay_String) { .length = strlen(Chat_Get_Message_At(game_data->chat_buffers, i)), .chars = Chat_Get_Message_At(game_data->chat_buffers, i) }), { .fontSize = 24, .textColor = {255, 255, 255, 255} });
                            }
                        }

                        Clay_ScrollContainerData scrollContainerData = Clay_GetScrollContainerData(CLAY_ID("ChatHistoryContainer"));

                        // If we're at the end, lock scroll to the end
                        if (scrollContainerData.scrollPosition->y == previousChatBottom) // Could scrollposition be null here?
                        {
                            float bottomPosition = -(scrollContainerData.contentDimensions.height - scrollContainerData.scrollContainerDimensions.height);
                            scrollContainerData.scrollPosition->y = bottomPosition;
                        }

                        previousChatBottom = -(scrollContainerData.contentDimensions.height - scrollContainerData.scrollContainerDimensions.height);
                    }

                    float chatInputBoxAlpha = 100;
                    if (chatting)
                    {
                        // if chatting, make the carrot more visible
                        chatInputBoxAlpha = 200;
                    }

                    CLAY(CLAY_ID("ChatInputBox"), { .layout = { .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) }, .layoutDirection = CLAY_LEFT_TO_RIGHT, .padding = CLAY_PADDING_ALL(5), .childGap = 3, .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER } }, .backgroundColor = { 50, 50, 50, chatInputBoxAlpha } }) {
                        float carrotAlpha = 150;
                        if (chatting)
                        {
                            // if chatting, make the carrot more visible
                            carrotAlpha = 255;
                        }
                        CLAY_TEXT(CLAY_STRING(">"), { .fontSize = 24, .textColor = {255, 255, 255, carrotAlpha} });
                        CLAY_TEXT(((Clay_String) { .length = strlen(game_data->chat_buffers->chat_input_buffer), .chars = game_data->chat_buffers->chat_input_buffer }), { .fontSize = 24, .textColor = {255, 255, 255, 255} });
                    }
                }
            }
        }
    }

    return Clay_EndLayout(delta_time);
}