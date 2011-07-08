/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2008-2011 WiredTiger, Inc.
 *	All rights reserved.
 */

#include "wt_internal.h"

/*
 * __wt_config_check--
 *	Check that all keys in an application-supplied config string match
 *	what is specified in the check string.
 *
 * All check strings are generated by dist/config.py from the constraints given
 * in dist/api_data.py
 */
int
__wt_config_check(WT_SESSION_IMPL *session,
    const char *checks, const char *config)
{
	WT_CONFIG parser, cparser, sparser;
	WT_CONFIG_ITEM k, v, chk, ck, cv, dummy;
	int found, ret;

	/* It is always okay to pass NULL. */
	if (config == NULL)
		return (0);

	WT_RET(__wt_config_init(session, &parser, config));
	while ((ret = __wt_config_next(&parser, &k, &v)) == 0) {
		if (k.type != ITEM_STRING && k.type != ITEM_ID) {
			__wt_errx(session,
			    "Invalid configuration key found: '%.*s'",
			    (int)k.len, k.str);
			return (EINVAL);
		}

		if ((ret = __wt_config_getone(session,
		    checks, &k, &chk)) != 0) {
			if (ret == WT_NOTFOUND) {
				__wt_errx(session,
				    "Unknown configuration key found: '%.*s'",
				    (int)k.len, k.str);
				ret = EINVAL;
			}
			return (ret);
		}

		WT_RET(__wt_config_subinit(session, &cparser, &chk));
		while ((ret = __wt_config_next(&cparser, &ck, &cv)) == 0) {
			if (strncmp(ck.str, "type", ck.len) == 0) {
				if ((strncmp(cv.str, "int", cv.len) == 0 &&
				    v.type != ITEM_NUM) ||
				    (strncmp(cv.str, "boolean", cv.len) == 0 &&
				    (v.type != ITEM_NUM ||
				    (v.val != 0 && v.val != 1))) ||
				    (strncmp(cv.str, "list", cv.len) == 0 &&
				    v.type != ITEM_STRUCT)) {
					__wt_errx(session, "Invalid value type "
					    "for key '%.*s': expected a %.*s",
					    (int)k.len, k.str,
					    (int)cv.len, cv.str);
					return (EINVAL);
				}
			} else if (strncmp(ck.str, "min", ck.len) == 0) {
				if (v.val < cv.val) {
					__wt_errx(session, "Value too small "
					    "for key '%.*s' "
					    "the minimum is %.*s",
					    (int)k.len, k.str,
					    (int)cv.len, cv.str);
					return (EINVAL);
				}
			} else if (strncmp(ck.str, "max", ck.len) == 0) {
				if (v.val > cv.val) {
					__wt_errx(session, "Value too large "
					    "for key '%.*s' "
					    "the maximum is %.*s",
					    (int)k.len, k.str,
					    (int)cv.len, cv.str);
					return (EINVAL);
				}
			} else if (strncmp(ck.str, "choices", ck.len) == 0) {
				if (v.type == ITEM_STRUCT) {
					/*
					 * Handle the 'verbose' case of a list
					 * containing restricted choices.
					 */
					WT_RET(__wt_config_subinit(session,
					    &sparser, &v));
					found = 1;
					while (found &&
					    (ret = __wt_config_next(&sparser,
					    &v, &dummy)) == 0) {
						ret = __wt_config_subgetraw(
						    session, &cv, &v, &dummy);
						found = (ret == 0);
					}
				} else  {
					ret = __wt_config_subgetraw(session,
					    &cv, &v, &dummy);
					found = (ret == 0);
				}

				if (ret != 0 && ret != WT_NOTFOUND)
					return (ret);
				if (!found) {
					__wt_errx(session, "Value '%.*s' not a "
					    "permitted choice for key '%.*s'",
					    (int)v.len, v.str,
					    (int)k.len, k.str);
					return (EINVAL);
				}
			} else
				WT_ASSERT(session, ck.str != ck.str);
		}
	}

	if (ret == WT_NOTFOUND)
		ret = 0;

	return (ret);
}
