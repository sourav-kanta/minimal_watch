#include <lvgl.h>

/**
 * @brief Finds first child of a lv_obj by BFS that has been added to the navigation
 * group, this is needed as immideate child may not be the next nav stop 
 *
 * @param root lv_obj whose focusable child we want to find
 *
 * @return 
 */
lv_obj_t* find_first_focusable_bfs(lv_obj_t* root) {
    if (!root) return NULL;

    // Level 1: Check all immediate children first (Breadth)
    uint32_t cnt = lv_obj_get_child_cnt(root);
    for (uint32_t i = 0; i < cnt; i++) {
        lv_obj_t* child = lv_obj_get_child(root, i);
        if (lv_obj_get_group(child) == lv_group_get_default()) {
            return child; // Found at the closest level
        }
    }

    // Level 2+: If none found, recurse into children's children
    for (uint32_t i = 0; i < cnt; i++) {
        lv_obj_t* found = find_first_focusable_bfs(lv_obj_get_child(root, i));
        if (found) return found;
    }

    return NULL;
}

/**
 * @brief Helper function that removes border, padding and outline
 *
 * @param obj
 */
void remove_shadow_and_outline(lv_obj_t* obj) {
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(obj, 0,LV_PART_MAIN);
    lv_obj_set_style_pad_row(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(obj, 0, 0);
}

void make_obj_navigable(lv_obj_t* obj) {
    lv_group_add_obj(lv_group_get_default(), obj);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);
}

