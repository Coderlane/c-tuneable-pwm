/**
 * @file bbb_pwm.h
 * @brief 
 * @author Travis Lane
 * @version 
 * @date 2014-10-10
 */

enum bbb_pwm_state_e {
	BPS_FREE = 0,
	BPS_BUSY = 1
};

enum bbb_pwm_return_code_e {
	BPRC_NOT_IMPLEMENTED = -100,
	BPRC_NO_CAPEMGR = -50,
	BPRC_ERROR = -2,
	BPRC_OK = 0
};

struct bbb_pwm_t;
struct bbb_pwm_controller_t;

struct bbb_pwm_controller_t* bbb_pwm_controller_new();
void bbb_pwm_controller_delete(struct bbb_pwm_controller_t** bpc_ptr);

const struct bbb_pwm_t* 
bbb_pwm_controller_get(struct bbb_pwm_controller_t* bpc, int pwm_id);