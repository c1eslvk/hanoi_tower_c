#include "primlib.h"
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#define PEGS_NUM 5
#define DISCS_NUM 60
#define FLOOR_HEIGHT 15
#define PEG_DISTANCE 500
#define PEG_WIDTH 5
#define PEG_SHIFT 100
#define TOP_BOUNDARY 100
#define STEP 5
#define DELAY 10

typedef struct {
	float width;
	float height;
	float x;
	float y;
} Disc;

typedef struct {
    int top;
    Disc data[DISCS_NUM];
} Stack;

typedef struct {
	float position;
	Stack discs;
} Peg;

typedef struct {
	Peg pegs[PEGS_NUM];
	Disc discs[DISCS_NUM];
	Disc *moved_disc;
	Peg *target_peg;
	int is_animating;
} Context;

void push(Stack *, Disc *);
Disc *pop(Stack *);
Disc *peek(Stack *);
void draw_floor();
void init_context(Context *);
void init_pegs(Context *);
void init_discs(Context *);
void draw_pegs(Context *);
void draw_discs(Context *, Peg *);
void wait_for_key(Context *);
int get_peg_idx(int);
void move_discs(Context *);
void run(Context *);
int check_win(Context *);

void push(Stack *stack, Disc *disc) {
	if (stack->top < DISCS_NUM)
		stack->data[stack->top++] = *disc;
}

Disc *pop(Stack *stack) {
    if (stack->top > 0) 
    	return &stack->data[--stack->top];
	else
		return NULL;
}

Disc *peek(Stack *stack) {
    if (stack->top > 0)
    	return &stack->data[stack->top - 1];
	else
		return NULL;
}

void draw_floor() {
	gfx_filledRect(0, gfx_screenHeight() - FLOOR_HEIGHT, gfx_screenWidth(), gfx_screenHeight(), YELLOW);
}

void init_context(Context *context) {
	context->moved_disc = NULL;
	context->target_peg = NULL;
	context->is_animating = 0;
}

void init_pegs(Context *context) {
	for(int peg_ct = 0; peg_ct < PEGS_NUM; peg_ct++){
		Peg *peg = &context->pegs[peg_ct];
		peg->position = (gfx_screenWidth() / (PEGS_NUM + 1)) * (peg_ct + 1);
		peg->discs.top = 0;
	}
}

void init_discs(Context *context) {
	float base_width = context->pegs[1].position - context->pegs[0].position;
	float width_change = (base_width - (2 * PEG_WIDTH)) / DISCS_NUM;
	float disc_height = (PEG_DISTANCE - PEG_SHIFT) / DISCS_NUM;
	Peg *peg = &context->pegs[0];

	for (int dc_count = 0; dc_count < DISCS_NUM; dc_count++) {
		Disc *disc = &context->discs[dc_count];
		disc->width = base_width - (width_change * dc_count);
		disc->height = disc_height;
		disc->y = gfx_screenHeight() - FLOOR_HEIGHT - (disc->height * (dc_count + 1));
		disc->x = peg->position + (PEG_WIDTH/2) - disc->width / 2;

		push(&peg->discs, disc);
	}
}

void draw_pegs(Context *context) {
	for(int peg_ct = 0; peg_ct < PEGS_NUM; peg_ct++) {
		Peg *peg = &context->pegs[peg_ct];
		float x = peg->position;
		gfx_filledRect(x, gfx_screenHeight() - PEG_DISTANCE, x + PEG_WIDTH, gfx_screenHeight() - FLOOR_HEIGHT, YELLOW);
		draw_discs(context, peg);
	}
}

void draw_discs(Context *context, Peg *peg) {
	for (int dc_count = 0; dc_count < peg->discs.top; dc_count ++) {		
		Disc *disc = &peg->discs.data[dc_count];

		gfx_filledRect(disc->x, disc->y, disc->x + disc->width, disc->y + disc->height, (context->moved_disc == disc) ? MAGENTA : RED);
	}

	if (context->moved_disc != NULL) {
		Disc *disc = context->moved_disc;

		gfx_filledRect(disc->x, disc->y, disc->x + disc->width, disc->y + disc->height, (context->moved_disc == disc) ? MAGENTA : RED);
	}
}

void run(Context *context) {
	int run = 1;

	while (run == 1) {
    	gfx_filledRect(0, 0, gfx_screenWidth() - 1, gfx_screenHeight() - 1, BLACK);
    	draw_floor();
		draw_pegs(context);

		if (gfx_isKeyDown(SDLK_SPACE) == 1)
			run = 0;

		if (check_win(context) == 1){
			gfx_textout((gfx_screenWidth() / 2 - 30), gfx_screenHeight() / 5, "YOU WIN!!!", GREEN);
			run = 0;
		}

 		gfx_updateScreen();
  		SDL_Delay(DELAY);
		wait_for_key(context);
	}
	gfx_getkey();
}

void wait_for_key(Context *context) {
	if (context->is_animating) {
		move_discs(context);
	}
	else {
		int pressed_key = gfx_getkey();
		
		if (pressed_key >= SDLK_0 && pressed_key <= SDLK_9) {
			int peg_idx = get_peg_idx(pressed_key);

			if (peg_idx >= 0) {
				Peg *peg = &context->pegs[peg_idx];

				if (context->moved_disc == NULL) {
					if (peek(&peg->discs) != NULL) {
						context->moved_disc = pop(&peg->discs);
					}
				}
				else {
					Disc *from = context->moved_disc;
					Disc *to = peek(&peg->discs);
					if (to == NULL || to->width > from->width) {
						context->target_peg = peg;
						context->is_animating = 1;
					}
				}
			}
		}
	}
}

int get_peg_idx(int pressed_key) {
	int idx = 9;
	if (pressed_key != SDLK_0)
		idx = pressed_key - SDLK_1;

	if (idx >= 0 && idx < PEGS_NUM)
		return idx;
	else
		return -1;
}

void move_discs(Context *context) {
	if (context->moved_disc != NULL && context->target_peg != NULL && context->is_animating) {
		Disc *target_top = peek(&context->target_peg->discs);
		float target_top_y = target_top != NULL ? target_top->y : gfx_screenHeight() - FLOOR_HEIGHT;
		float destination_x = context->target_peg->position - context->moved_disc->width / 2 + PEG_WIDTH / 2;
		float destination_y = target_top_y - context->moved_disc->height;

		if (context->moved_disc->x != destination_x && context->moved_disc->y > TOP_BOUNDARY) { //Move up
			context->moved_disc->y -= STEP;

			if (context->moved_disc->y < TOP_BOUNDARY)
				context->moved_disc->y = TOP_BOUNDARY;
		}
		else if (context->moved_disc->x == destination_x && context->moved_disc->y < destination_y) { //Move down
			context->moved_disc->y += STEP;

			if (context->moved_disc->y > destination_y)
				context->moved_disc->y = destination_y;
		}
		else if (context->moved_disc->x > destination_x && context->moved_disc->y == TOP_BOUNDARY) { //Move left
			context->moved_disc->x -= STEP;

			if (context->moved_disc->x < destination_x)
				context->moved_disc->x = destination_x;
		}
		else if (context->moved_disc->x < destination_x && context->moved_disc->y == TOP_BOUNDARY) { //Move right
			context->moved_disc->x += STEP;

			if (context->moved_disc->x > destination_x)
				context->moved_disc->x = destination_x;
		}
		else { //On target
			context->is_animating = 0;
			push(&context->target_peg->discs, context->moved_disc);
			context->target_peg = NULL;
			context->moved_disc = NULL;
		}
	}
}

int check_win(Context *context) {
	if (context->pegs[PEGS_NUM - 1].discs.top == DISCS_NUM) {
		return 1;
	}
	else {
		return 0;
	}
}

int main(int argc, char *argv[]) {
	if (gfx_init())
    	exit(3);

	Context context;

	init_context(&context);
	init_pegs(&context);
	init_discs(&context);
	run(&context);

	return 0;
}
