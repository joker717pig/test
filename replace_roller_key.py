import sys

filepath = 'main/ui_page_cn/SetPage/PelFlo_Set_ui_event_cb.c'

with open(filepath, 'r', encoding='utf-8') as f:
    content = f.read()

old = '''    if (key != LV_KEY_ENTER) return;

    lv_obj_t* target = NULL;
    if (obj == Set_Widget.Roller_Year)       target = Set_Widget.Roller_Month;
    else if (obj == Set_Widget.Roller_Month) target = Set_Widget.Roller_Day;
    else if (obj == Set_Widget.Roller_Day)   target = Set_Widget.Btn_Page4Next;
    else if (obj == Set_Widget.Roller_Hour)  target = Set_Widget.Roller_Min;
    else if (obj == Set_Widget.Roller_Min)   target = Set_Widget.Btn_Sure;
    if (target && lv_obj_is_valid(target)) {
        lv_group_focus_obj(target);
        lv_event_stop_processing(e);
    }'''

new = '''    if (key == LV_KEY_LEFT || key == LV_KEY_RIGHT) {
        lv_obj_t* target = NULL;
        if (obj == Set_Widget.Roller_Year) {
            if (key == LV_KEY_RIGHT) target = Set_Widget.Roller_Month;
        } else if (obj == Set_Widget.Roller_Month) {
            if (key == LV_KEY_RIGHT) target = Set_Widget.Roller_Day;
            else if (key == LV_KEY_LEFT) target = Set_Widget.Roller_Year;
        } else if (obj == Set_Widget.Roller_Day) {
            if (key == LV_KEY_LEFT) target = Set_Widget.Roller_Month;
        } else if (obj == Set_Widget.Roller_Hour) {
            if (key == LV_KEY_RIGHT) target = Set_Widget.Roller_Min;
        } else if (obj == Set_Widget.Roller_Min) {
            if (key == LV_KEY_LEFT) target = Set_Widget.Roller_Hour;
        }
        if (target && lv_obj_is_valid(target)) {
            lv_group_focus_obj(target);
            lv_event_stop_processing(e);
        }
        return;
    }
    
    if (key == LV_KEY_ENTER) {
        lv_obj_t* target = NULL;
        if (obj == Set_Widget.Roller_Day)   target = Set_Widget.Btn_Page4Next;
        else if (obj == Set_Widget.Roller_Min) target = Set_Widget.Btn_Sure;
        if (target && lv_obj_is_valid(target)) {
            lv_group_focus_obj(target);
            lv_event_stop_processing(e);
        }
    }'''

if old in content:
    content = content.replace(old, new)
    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(content)
    print('SUCCESS: Replaced roller key logic')
else:
    print('ERROR: Old string not found')
    # Print the actual content around line 732 for debugging
    lines = content.split('\n')
    for i in range(730, min(745, len(lines))):
        print(f'Line {i+1}: {repr(lines[i])}')