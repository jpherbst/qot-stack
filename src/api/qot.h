#ifndef QOT_H
#define QOT_H

#include <stdint.h>

/**
 * @brief Error codes for functions
 */
typedef enum
{
	SUCCESS            =  0,
	NO_SCHEDULER_CHDEV = -1,	 /**< No scheduler character device */
	INVALID_BINDING_ID = -2,	 /**< Invalid binding ID specified */
	INVALID_UUID       = -3,	 /**< Invalid timeline UUID specified */
	IOCTL_ERROR        = -4		 /**< IOCTL returned error */
} 
qot_error;

/**
 * @brief Bind to a timeline
 * @param timeline A unique identifier for the timeline, eg. "sample_timeline"
 * @param accuracy The desired accuracy in nanoseconds
 * @param resolution The desired resolution in nanoseconds
 * @return A strictly non-negative ID for the binding. A negative number
 *         indicates an error
 *
 **/
int32_t qot_bind(const char *timeline, uint64_t accuracy, uint64_t resolution);

/**
 * @brief Unbind from a timeline
 * @param bid A binding ID. Invalid IDs will be quietly ignored.
 * @return Success value -- negative:error code, positive:success
 **/
int32_t qot_unbind(int32_t bid);

/**
 * @brief Update a binding accuracy
 * @param bid A binding ID. Invalid IDs will be quietly ignored.
 * @param accuracy The new accuracy
 **/
int32_t qot_set_desired_accuracy(int32_t bid, uint64_t accuracy);

/**
 * @brief Update a binding resolution
 * @param bid A binding ID. Invalid IDs will be quietly ignored.
 * @param accuracy The new resolution
 **/
int32_t qot_set_desired_resolution(int32_t bid, uint64_t resolution);

/**
 * @brief Update a binding accuracy
 * @param bid A binding ID. Invalid IDs will be quietly ignored.
 * @param accuracy The current accuracy being enforced by sync service
* @return Success value -- negative:error, positive:success
 **/
int32_t qot_get_target_accuracy(int32_t bid, uint64_t *accuracy);

/**
 * @brief Update a binding resolution
 * @param bid A binding ID. Invalid IDs will be quietly ignored.
 * @param accuracy The current resolution being enforced by sync service
 * @return Success value -- negative:error, positive:success
 **/
int32_t qot_get_target_resolution(int32_t bid, uint64_t *resolution);


#endif
