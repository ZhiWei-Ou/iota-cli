#ifndef MAIN_H_
#define MAIN_H_
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define CLI_PROMPT \
"  _  ____ _____ ____        ____  _     _ \n" \
" / \\/  _ Y__ __Y  _ \\      /   _\\/ \\   / \\\n" \
" | || / \\| / \\ | / \\|_____ |  /  | |   | |\n" \
" | || \\_/| | | | |-||\\____\\|  \\__| |_/\\| |\n" \
" \\_/\\____/ \\_/ \\_/ \\|      \\____/\\____/\\_/\n"\

void register_feature_function(int (*)());

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* MAIN_H_ */
