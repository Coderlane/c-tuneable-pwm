/**
 * @file bbb_pwm.c
 * @brief 
 * @author Travis Lane
 * @version 
 * @date 2014-10-10
 */

#include <bbb_pwm_internal.h>

/**
 * @brief Create a new controller.
 *
 * @return A new controller, or NULL on failure.
 */
struct bbb_pwm_controller_t* 
bbb_pwm_controller_new()
{
	struct bbb_pwm_controller_t* bpc = NULL;

	bpc = calloc(sizeof(struct bbb_pwm_controller_t), 1);
	assert(bpc != NULL);

	if(bbb_pwm_controller_init(bpc) != BPRC_OK) {
		bbb_pwm_controller_delete(&bpc);
		goto out;
	}

	bbb_pwm_controller_probe(bpc);

out:
	return bpc;
}

/**
 * @brief Free a controller.
 *
 * @param bpc_ptr The controller to free.
 */
void 
bbb_pwm_controller_delete(struct bbb_pwm_controller_t **bpc_ptr) 
{
	struct bbb_pwm_controller_t *bpc;

	// Check the ptr.
	if(bpc_ptr == NULL) {
		return;
	}
	// Check the referenced ptr.
	bpc = (*bpc_ptr);
	if(bpc == NULL) {
		return;
	}

	while(bpc->bpc_head_pwm) {
		// Unclaim before we remove it.
		bbb_pwm_unclaim(bpc->bpc_head_pwm);
		bbb_pwm_controller_remove_pwm(bpc, bpc->bpc_head_pwm->bp_name);
	}

	// Free the origional bpc
	free(bpc);
	(*bpc_ptr) = NULL;
}

/**
 * @brief Initializes a bbb pwm controller.
 *
 * @param bpc The bbb pwm controller to initialize.
 *
 * @return A status code.
 */
int
bbb_pwm_controller_init(struct bbb_pwm_controller_t *bpc)
{
	// Create a new bbb_capemgr to talk to the capemanager.
	bpc->bpc_capemgr = bbb_capemgr_new();
	if(bpc->bpc_capemgr == NULL) {
		return BPRC_NO_CAPEMGR;
	}	
	
	// Enable the am33xx pwm controller on the cape manager.
	if(bbb_capemgr_enable(bpc->bpc_capemgr, "am33xx_pwm") < 0) {
		return BPRC_NO_PWM;
	}

	// Probing can fail and it isn't the end of the world.
	// Maybe emit a warning?
	bbb_pwm_controller_probe(bpc);

	return BPRC_OK;
}

/**
 * @brief Probe the filesystem for obvious PWM devices.
 *
 * @param bpc The controller to probe on.
 *
 * @return A status code.
 */
int 
bbb_pwm_controller_probe(struct bbb_pwm_controller_t* bpc)
{
	assert(bpc != NULL);
	// TODO: Implement probing for pwms on the local filesystem.	
	return BPRC_NOT_IMPLEMENTED;
}

/**
 * @brief Adds a PWM to a controller. 
 *
 * @param bpc The controller to add to.
 * @param bp The PWM to add.
 *
 * @return A status code.
 */
int 
bbb_pwm_controller_add_pwm(struct bbb_pwm_controller_t* bpc, 
		struct bbb_pwm_t* bp)
{
	struct bbb_pwm_t* cur = NULL;
	int result;

	assert(bpc != NULL);
	assert(bp != NULL);

	// Linked list insert in order.
	cur = bpc->bpc_head_pwm;
	if(cur == NULL) {
		// New list.
		bpc->bpc_head_pwm = bp;
		return BPRC_OK;
	}

	// Do our compare.
	result = strcmp(bp->bp_name, cur->bp_name);

	if(result == 0) {
		// Front was duplicate.
		return BPRC_DUPLICATE;
	}

	if(result < 0) {
		// Insert in front.
		bp->bp_next = cur;
		bpc->bpc_head_pwm = bp;
		return BPRC_OK;
	}

	// Not front or dupe, thus we must search!
	while(cur->bp_next != NULL) {
		// Do our compare.
		// Note we compare on the NEXT element.
		result = strcmp(bp->bp_name, cur->bp_next->bp_name);

		if(result == 0) {
			// Found a duplicate item.
			return BPRC_DUPLICATE;
		}

		if(result < 0) {
			// Found our place.
			bp->bp_next = cur->bp_next;
			cur->bp_next = bp;
			return BPRC_OK;
		}
		// This is not the pointer you were looking for.
		cur = cur->bp_next;
	}
	// If hit the end and we still haven't found our place, insert at end.
	cur->bp_next = bp;
	return BPRC_OK;
}

/**
 * @brief Removes a PWM from a controller.
 * Note, the PWM must not be in use.
 *
 * @param bpc The comtroller to remove the pwm from.
 * @param name The name of the PWM to remove.
 *
 * @return A status code.
 */
int 
bbb_pwm_controller_remove_pwm(struct bbb_pwm_controller_t* bpc,
		const char* name)
{
	struct bbb_pwm_t* cur;
	int result;

	assert(bpc != NULL);
	assert(name != NULL);

	// Remove from a sorted list

	cur = bpc->bpc_head_pwm;
	if(cur == NULL) {
		// Empty list.
		return BPRC_PWM_NOT_FOUND;
	}

	// Do our compare
	result = strcmp(cur->bp_name, name);

	if(result == 0) {
		if(!bbb_pwm_is_unclaimed(cur)) {
			// Make sure it isn't locked.
			return BPRC_BUSY;
		}
		// First item was the one to delete.
		bpc->bpc_head_pwm = cur->bp_next;
		bbb_pwm_delete(&cur);
		bpc->bpc_num_pwms--;
		return BPRC_OK;
	}

	if(result > 0) {
		// Item can't possibly be in our list.
		return BPRC_PWM_NOT_FOUND;
	}

	while(cur->bp_next != NULL) {
		// Do our compare.
		result = strcmp(cur->bp_next->bp_name, name);
		if(result == 0) {
			// Found our target.
			struct bbb_pwm_t* tmp = cur->bp_next;

			if(!bbb_pwm_is_unclaimed(cur)) {
				// Make sure it isn't locked.
				return BPRC_BUSY;
			}

			// Remove it from the linked list.
			cur->bp_next = tmp->bp_next;
			bbb_pwm_delete(&tmp);
			bpc->bpc_num_pwms--;

			return BPRC_OK;
		}

		if(result > 0) {
			// Item can't possibly be in our list.
			return BPRC_PWM_NOT_FOUND;
		}
		// Move on.
		cur = cur->bp_next;
		assert(cur != NULL);
	}

	// We are at the end and didn't find anything.
	return BPRC_PWM_NOT_FOUND;
}

/**
 * @brief Creates a new bbb_pwm_t.
 *
 * @param name The name of the new pwm.
 *
 * @return A new pwm or NULL on failure.
 */
struct bbb_pwm_t*
bbb_pwm_new(const char* name, const char* root_path) 
{
	struct bbb_pwm_t* bp;
	if(name == NULL) {
		return NULL;
	}

	bp = calloc(sizeof(struct bbb_pwm_t), 1);
	assert(bp != NULL);

	// Initially we are unclaimed.
	bp->bp_state = BPS_UNCLAIMED;
	// Copy the name
	bp->bp_name = (char*) strdup(name);
	// Setup our paths.
	asprintf(&(bp->bp_duty_file_path), "%s/%s", root_path, "duty");
	asprintf(&(bp->bp_period_file_path), "%s/%s", root_path, "period");
	asprintf(&(bp->bp_polarity_file_path), "%s/%s", root_path, "polarity");
	
	assert(bp->bp_name != NULL);

	return bp;
}

/**
 * @brief Deletes a PWM.
 * This will unclaim if claimed.
 *
 * @param bp_ptr The pwm to delete.
 */
void
bbb_pwm_delete(struct bbb_pwm_t** bp_ptr) 
{
	struct bbb_pwm_t* bp;
	assert(bp_ptr != NULL);

	bp = *bp_ptr;
	if(bp == NULL) {
		return;
	}

	// Unclaim, since we are freeing.	
	bbb_pwm_unclaim(bp);	
	
	// Free what has been allocated. 
	if(bp->bp_name != NULL) {
		free(bp->bp_name);
	}

	if(bp->bp_duty_file_path != NULL) {
		free(bp->bp_duty_file_path);
	}

	if(bp->bp_period_file_path != NULL) {
		free(bp->bp_period_file_path);
	}

	if(bp->bp_polarity_file_path != NULL) {
		free(bp->bp_polarity_file_path);
	}

	free(bp);
	*bp_ptr = NULL;	
}

/**
 * @brief Claims a pwm for later setting values.
 * If someone else has claimed this pwm, we fail and report BUSY.
 *
 * @param bp The pwm to claim.
 *
 * @return A status code.
 */
int 
bbb_pwm_claim(struct bbb_pwm_t* bp)
{
	int result = BPRC_OK;

	assert(bp != NULL);

	if(bbb_pwm_is_claimed(bp)) {
		// We already claimed it.
		return BPRC_OK;
	}

	assert(bp->bp_duty_file_path != NULL);
	assert(bp->bp_period_file_path != NULL);
	assert(bp->bp_polarity_file_path != NULL);

	// Open the necessary files.
	bp->bp_duty_file = fopen(bp->bp_duty_file_path, "r+");
	if(bp->bp_duty_file == NULL) {
		result = BPRC_BUSY;
		goto out;
	}
	
	bp->bp_period_file = fopen(bp->bp_period_file_path, "r+");
	if(bp->bp_period_file == NULL) {
		result = BPRC_BUSY;
		goto out;
	}
	
	bp->bp_polarity_file = fopen(bp->bp_polarity_file_path, "r+");
	if(bp->bp_polarity_file == NULL) {
		result = BPRC_BUSY;
		goto out;
	}

	// Load the cached values.
	result = read_uint32_from_file(bp->bp_duty_file, &(bp->bp_duty_cycle));
	if(result != BPRC_OK) {
		goto out;
	}
	
	result = read_uint32_from_file(bp->bp_period_file, &(bp->bp_period));
	if(result != BPRC_OK) {
		goto out;
	}
	
	result = read_int8_from_file(bp->bp_polarity_file, &(bp->bp_polarity));
	if(result != BPRC_OK) {
		goto out;
	}

out:
	if(result != BPRC_OK) {
		// On failure, unclaim will force a cleanup.
		bbb_pwm_unclaim(bp);
	} else {
		bp->bp_state = BPS_CLAIMED;
	}
	return result;
}

/**
 * @brief Unclaims a pwm.
 *
 * @param bp The pwm to unclaim.
 *
 * @return A status code.
 */
int 
bbb_pwm_unclaim(struct bbb_pwm_t* bp)
{
	assert(bp != NULL);

	bp->bp_state = BPS_UNCLAIMED;
	// Close the duty file.	
	if(bp->bp_duty_file != NULL) {
		fclose(bp->bp_duty_file);
		bp->bp_duty_file = NULL;
	}
	// Close the period file.
	if(bp->bp_period_file != NULL) {
		fclose(bp->bp_period_file);
		bp->bp_period_file = NULL;
	}
	// Close the polarity file.
	if(bp->bp_polarity_file != NULL) {
		fclose(bp->bp_polarity_file);
		bp->bp_polarity_file = NULL;
	}
	return BPRC_OK;
}

/**
 * @brief Check to see if the pwm is unclaimed.
 *
 * @param bp The pwm to check.
 *
 * @return True/False is the pwm unclaimed.
 */
int 
bbb_pwm_is_unclaimed(struct bbb_pwm_t* bp)
{
	assert(bp != NULL);
	return bp->bp_state == BPS_UNCLAIMED;
}

/**
 * @brief Check to see if we have claimership of the pwm.
 *
 * @param bp The pwm to check.
 *
 * @return True/False if we have claimership.
 */
int
bbb_pwm_is_claimed(struct bbb_pwm_t* bp)
{
	assert(bp != NULL);
	return bp->bp_state == BPS_CLAIMED;
}

/**
 * @brief Set the duty of the pwm.
 * NOTE: The pwm must be claimed to set anything on it.
 *
 * @param bp The pwm to set.
 * @param duty The duty to set the pwm to.
 *
 * @return A status code.
 */
int 
bbb_pwm_set_duty_cycle(struct bbb_pwm_t* bp, uint32_t duty_cycle)
{
	int result;
	assert(bp != NULL);

	if(!bbb_pwm_is_claimed(bp)) {
		return BPRC_NOT_CLAIMED;
	}

	// Write the data.
	result = write_uint32_to_file(bp->bp_duty_file, duty_cycle);
	if(result != BPRC_OK) {
		return result;
	}

	bp->bp_duty_cycle = duty_cycle;
	return BPRC_OK;
}

/**
 * @brief Set the period of the pwm.
 * NOTE: The pwm must be claimed to set anything on it.
 *
 * @param bp The pwm to set.
 * @param period The period to set it to.
 *
 * @return A status code. 
 */
int 
bbb_pwm_set_period(struct bbb_pwm_t* bp, uint32_t period)
{
	int result;
	assert(bp != NULL);

	if(!bbb_pwm_is_claimed(bp)) {
		return BPRC_NOT_CLAIMED;
	}

	result = write_uint32_to_file(bp->bp_period_file, period);
	if(result != BPRC_OK) {
		return result;
	}
	
	bp->bp_period = period;
	return BPRC_OK;
}

/**
 * @brief Sets the polarity of the pwm.
 * NOTE: The pwm must be claimed to set anything on it.
 *
 * @param bp The pwm to set.
 * @param polarity The polarity to set the pwm to.
 *
 * @return A status code.
 */
int 
bbb_pwm_set_polarity(struct bbb_pwm_t* bp, int8_t polarity)
{
	int result;
	assert(bp != NULL);

	if(!bbb_pwm_is_claimed(bp)) {
		return BPRC_NOT_CLAIMED;
	}

	// TODO: Do I need to disable the pwm first?
	// https://www.kernel.org/doc/Documentation/pwm.txt

	if(polarity != -1 && polarity != 1) {
		// TODO Verify these limits.
		return BPRC_RANGE;
	}

	result = write_uint32_to_file(bp->bp_polarity_file, polarity);
	if(result != BPRC_OK) {
		return result;
	}

	bp->bp_polarity = polarity;
	return BPRC_OK;
}

/**
 * @brief 
 *
 * @param bp
 * @param percent
 *
 * @return 
 */
int 
bbb_pwm_set_duty_percent(struct bbb_pwm_t* bp, float percent)
{
	uint32_t duty_cycle, period;
	int result;
	assert(bp != NULL);

	if(!bbb_pwm_is_claimed(bp)) {
		return BPRC_NOT_CLAIMED;
	}

	if(percent < 0.0f || percent > 100.0f) {
		return BPRC_RANGE;
	}
	
	result = bbb_pwm_get_period(bp, &period);
	if(result != BPRC_OK) {
		return result;
	}
	// We need to invert the percentage.
	// 0 Should be FULL STOP 100 should be FULL SPEED
	duty_cycle = (uint32_t)(((float) period) * (percent / 100.0f)); 

	return bbb_pwm_set_duty_cycle(bp, duty_cycle);
}

/**
 * @brief
 *
 * @param bp
 * @param hertz
 *
 * @return 
 */
int 
bbb_pwm_set_frequency(struct bbb_pwm_t* bp, uint32_t hertz)
{
	int result;
	uint32_t period;
	uint32_t duty;

	assert(bp != NULL);

	if(!bbb_pwm_is_claimed(bp)) {
		return BPRC_NOT_CLAIMED;
	}

	// period can't be less than 1
	// rule out divide by zero.
	if(hertz > 1e9 || hertz <= 0) {
		return BPRC_RANGE;
	}

	// Convert hertz to period in nanoseconds.
	period = 1e9 / hertz;

	result = bbb_pwm_get_duty_cycle(bp, &duty);
	if(result != BPRC_OK) {
		return result;
	}

	// Duty can't exceede period.
	// Duty should probably be throttled to zero first!
	// Weird shit might happen :/
	if(period < duty) {
		return BPRC_RANGE;
	}

	// Set the new period.
	return bbb_pwm_set_period(bp, period);
}

/**
 * @brief Gets the duty cycle of a pwm.
 * If the pwm isn't claimed, we attempt to open the right file for reading.
 * Note that this may fail if someone else owns it.
 *
 * @param bp The pwm to read from.
 * @param[out] out_duty The duty cycle read.
 *
 * @return A status code.
 */
int 
bbb_pwm_get_duty_cycle(struct bbb_pwm_t* bp, uint32_t* out_duty)
{
	FILE* duty_file = NULL;
	int result = BPRC_OK;
	
	assert(bp != NULL);
	assert(out_duty != NULL);

	if(bbb_pwm_is_claimed(bp)) {
		// Value was cached.
		*out_duty = bp->bp_duty_cycle;
		goto out;
	}

	duty_file = fopen(bp->bp_duty_file_path, "r");
	if(duty_file == NULL) {
		result = BPRC_BUSY;
		goto out;
	}
	
	result = read_uint32_from_file(duty_file, out_duty);
out:
	if(duty_file != NULL) {
		fclose(duty_file);
	}
	return result;
}

/**
 * @brief Gets the period of a pwm.
 * If the pwm isn't claimed, we attempt to open the right file for reading.
 * Note that this may fail if someone else owns it.
 * 
 * @param bp The pwm to read from.
 * @param[out] out_period The period read.
 * 
 * @return A status code.
 */
int 
bbb_pwm_get_period(struct bbb_pwm_t* bp, uint32_t* out_period)
{
	FILE* period_file = NULL;
	int result = BPRC_OK;

	assert(bp != NULL);
	assert(out_period != NULL);
	
	if(bbb_pwm_is_claimed(bp)) {
		*out_period = bp->bp_period;
		goto out;
	}

	period_file = fopen(bp->bp_period_file_path, "r");
	if(period_file == NULL) {
		result = BPRC_BUSY;
		goto out;
	}
	
	result = read_uint32_from_file(period_file, out_period);
out:
	if(period_file != NULL) {
		fclose(period_file);
	}
	return result;
}

/**
 * @brief Gets the polarity of a pwm. 
 * If the pwm isn't claimed, we attempt to open the right file for reading.
 * Note that this may fail if someone else owns it.
 *
 * @param bp The pwm to read from. 
 * @param[out] out_polarity The polarity read.
 *
 * @return A status code.
 */
int 
bbb_pwm_get_polarity(struct bbb_pwm_t* bp, int8_t* out_polarity)
{
	FILE* polarity_file = NULL;
	int result = BPRC_OK;

	assert(bp != NULL);
	assert(out_polarity != NULL);

	if(bbb_pwm_is_claimed(bp)) {
		*out_polarity = bp->bp_polarity;
		goto out;
	}

	polarity_file = fopen(bp->bp_polarity_file_path, "r");
	if(polarity_file == NULL) {
		result = BPRC_BUSY;
		goto out;
	}

	result = read_int8_from_file(polarity_file, out_polarity);
out:
	if(polarity_file != NULL) {
		fclose(polarity_file);
	}
	return result;
}

/**
 * @brief 
 *
 * @param bp
 * @param[out] out_percent
 *
 * @return 
 */
int 
bbb_pwm_get_duty_percent(struct bbb_pwm_t* bp, float* out_percent)
{
	uint32_t duty, period;
	int result;
	
	result = bbb_pwm_get_period(bp, &period);
	if(result != BPRC_OK) {
		return result;
	}
	
	result = bbb_pwm_get_duty_cycle(bp, &duty);
	if(result != BPRC_OK) {
		return result;
	}

	*out_percent = ((float) duty / (float) period) * 100.0f;
	return BPRC_OK;
}

/**
 * @brief 
 *
 * @param bp
 * @param[out] out_hertz
 *
 * @return 
 */
int 
bbb_pwm_get_frequency(struct bbb_pwm_t* bp, uint32_t* out_hertz)
{
	uint32_t period;
	int result;

	result = bbb_pwm_get_period(bp, &period);
	if(result != BPRC_OK) {
		return result;
	}
	
	// Convert nanoseconds to hertz.
	*out_hertz = 1e9 / period;
	return BPRC_OK;
}

/**
 * @brief 
 *
 * @param file
 *
 * @return 
 */
int 
can_write_to_file(FILE* file)
{
	int fd, mode;
	if(file == NULL) {
		return 0;
	}

	fd = fileno(file);
	if(fd < 0) {
		return 0;
	}

	mode = fcntl(fd, F_GETFL); 
	return ((mode & O_WRONLY) == O_WRONLY) || ((mode & O_RDWR) == O_RDWR);
}

/**
 * @brief 
 *
 * @param file
 *
 * @return 
 */
int 
can_read_from_file(FILE* file)
{
	int fd, mode;
	if(file == NULL) {
		return 0;
	}

	fd = fileno(file);
	if(fd < 0) {
		return 0;
	}

	mode = fcntl(fd, F_GETFL);
	return ((mode & O_WRONLY) != O_WRONLY) || ((mode & O_RDWR) == O_RDWR);
}

/**
 * @brief 
 *
 * @param file
 * @param[out] out_data
 *
 * @return 
 */
int 
read_uint32_from_file(FILE* file, uint32_t* out_data)
{
	int result;
	if(file == NULL || out_data == NULL) {
		return BPRC_NULL_PTR;
	}
	if(!can_read_from_file(file)) {
		return BPRC_BAD_FILE;
	}
	// Set to 0
	result = fseek(file, 0, SEEK_SET);
	if(result != 0) {
		return BPRC_BAD_FILE;
	}
	// Read the data.	
	result = fscanf(file, "%"PRIu32"", out_data);
	if(result < 0) {
		return BPRC_NO_DATA;
	}
	return BPRC_OK;
}

/**
 * @brief 
 *
 * @param file
 * @param[out] out_data
 *
 * @return 
 */
int 
read_int8_from_file(FILE* file, int8_t* out_data)
{
	int result;
	if(file == NULL || out_data == NULL) {
		return BPRC_NULL_PTR;
	}
	if(!can_read_from_file(file)) {
		return BPRC_BAD_FILE;
	}
	// Set to 0
	result = fseek(file, 0, SEEK_SET);
	if(result != 0) {
		return BPRC_BAD_FILE;
	}
	// Read the data.	
	result = fscanf(file, "%"SCNd8"", out_data);
	if(result < 0) {
		return BPRC_NO_DATA;
	}
	return BPRC_OK;
}

/**
 * @brief 
 *
 * @param file
 * @param data
 *
 * @return 
 */
int
write_uint32_to_file(FILE* file, uint32_t data)
{
	int result;
	if(file == NULL) {
		return BPRC_BAD_FILE;
	}
	if(!can_write_to_file(file)) {
		return BPRC_BAD_FILE;
	}
	// Truncate the file.
	if(freopen(NULL, "w+", file) == NULL) {
		return BPRC_BAD_FILE;
	}
	// Write the data
	result = fprintf(file, "%"PRIu32"", data);
	if(result <= 0) {
		return BPRC_BAD_WRITE;
	}
	return BPRC_OK;
}

/**
 * @brief 
 *
 * @param file
 * @param data
 *
 * @return 
 */
int 
write_int8_to_file(FILE* file, int8_t data)
{
	int result;
	if(file == NULL) {
		return BPRC_BAD_FILE;
	}
	if(!can_write_to_file(file)) {
		return BPRC_BAD_FILE;
	}
	// Truncate the file.
	if(freopen(NULL, "w+", file) == NULL) {
		return BPRC_BAD_FILE;
	}
	// Write the data
	result = fprintf(file, "%"PRId8"", data);
	if(result <= 0) {
		return BPRC_BAD_WRITE;	
	}
	return BPRC_OK;
}
