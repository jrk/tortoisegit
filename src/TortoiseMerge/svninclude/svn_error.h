/**
 * @copyright
 * ====================================================================
 * Copyright (c) 2000-2004, 2008 CollabNet.  All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.  The terms
 * are also available at http://subversion.tigris.org/license-1.html.
 * If newer versions of this license are posted there, you may use a
 * newer version instead, at your option.
 *
 * This software consists of voluntary contributions made by many
 * individuals.  For exact contribution history, see the revision
 * history and logs, available at http://subversion.tigris.org/.
 * ====================================================================
 * @endcopyright
 *
 * @file svn_error.h
 * @brief Common exception handling for Subversion.
 */




#ifndef SVN_ERROR_H
#define SVN_ERROR_H

#include <apr.h>
#include <apr_errno.h>     /* APR's error system */
#include <apr_pools.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define APR_WANT_STDIO
#endif
#include <apr_want.h>

#include "svn_types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** the best kind of (@c svn_error_t *) ! */
#define SVN_NO_ERROR   0

/* The actual error codes are kept in a separate file; see comments
   there for the reasons why. */
#include "svn_error_codes.h"

/** Set the error location for debug mode. */
void
svn_error__locate(const char *file,
                  long line);


/** Put an English description of @a statcode into @a buf and return @a buf,
 * NULL-terminated. @a statcode is either an svn error or apr error.
 */
char *
svn_strerror(apr_status_t statcode,
             char *buf,
             apr_size_t bufsize);


/** If @a err has a custom error message, return that, otherwise
 * store the generic error string associated with @a err->apr_err into
 * @a buf (terminating with NULL) and return @a buf.
 *
 * @since New in 1.4.
 *
 * @note @a buf and @a bufsize are provided in the interface so that
 * this function is thread-safe and yet does no allocation.
 */
const char *svn_err_best_message(svn_error_t *err,
                                 char *buf,
                                 apr_size_t bufsize);



/** SVN error creation and destruction.
 *
 * @defgroup svn_error_error_creation_destroy Error creation and destruction
 * @{
 */

/** Create a nested exception structure.
 *
 * Input:  an APR or SVN custom error code,
 *         a "child" error to wrap,
 *         a specific message
 *
 * Returns:  a new error structure (containing the old one).
 *
 * @note Errors are always allocated in a subpool of the global pool,
 *        since an error's lifetime is generally not related to the
 *        lifetime of any convenient pool.  Errors must be freed
 *        with svn_error_clear().  The specific message should be @c NULL
 *        if there is nothing to add to the general message associated
 *        with the error code.
 *
 *        If creating the "bottommost" error in a chain, pass @c NULL for
 *        the child argument.
 */
svn_error_t *
svn_error_create(apr_status_t apr_err,
                 svn_error_t *child,
                 const char *message);

/** Wrapper macro to collect file and line information */
#define svn_error_create \
  (svn_error__locate(__FILE__,__LINE__), (svn_error_create))

/** Create an error structure with the given @a apr_err and @a child,
 * with a printf-style error message produced by passing @a fmt, using
 * apr_psprintf().
 */
svn_error_t *
svn_error_createf(apr_status_t apr_err,
                  svn_error_t *child,
                  const char *fmt,
                  ...)
  __attribute__ ((format(printf, 3, 4)));

/** Wrapper macro to collect file and line information */
#define svn_error_createf \
  (svn_error__locate(__FILE__,__LINE__), (svn_error_createf))

/** Wrap a @a status from an APR function.  If @a fmt is NULL, this is
 * equivalent to svn_error_create(status,NULL,NULL).  Otherwise,
 * the error message is constructed by formatting @a fmt and the
 * following arguments according to apr_psprintf(), and then
 * appending ": " and the error message corresponding to @a status.
 * (If UTF-8 translation of the APR error message fails, the ": " and
 * APR error are not appended to the error message.)
 */
svn_error_t *
svn_error_wrap_apr(apr_status_t status,
                   const char *fmt,
                   ...)
       __attribute__((format(printf, 2, 3)));

/** Wrapper macro to collect file and line information */
#define svn_error_wrap_apr \
  (svn_error__locate(__FILE__,__LINE__), (svn_error_wrap_apr))

/** A quick n' easy way to create a wrapped exception with your own
 * message, before throwing it up the stack.  (It uses all of the
 * @a child's fields.)
 */
svn_error_t *
svn_error_quick_wrap(svn_error_t *child,
                     const char *new_msg);

/** Wrapper macro to collect file and line information */
#define svn_error_quick_wrap \
  (svn_error__locate(__FILE__,__LINE__), (svn_error_quick_wrap))

/** Add @a new_err to the end of @a chain's chain of errors.  The @a new_err
 * chain will be copied into @a chain's pool and destroyed, so @a new_err
 * itself becomes invalid after this function.
 */
void
svn_error_compose(svn_error_t *chain,
                  svn_error_t *new_err);

/** Return the root cause of @a err by finding the last error in its
 * chain (e.g. it or its children).  @a err may be @c SVN_NO_ERROR, in
 * which case @c SVN_NO_ERROR is returned.
 *
 * @since New in 1.5.
 */
svn_error_t *
svn_error_root_cause(svn_error_t *err);

/** Create a new error that is a deep copy of @a err and return it.
 *
 * @since New in 1.2.
 */
svn_error_t *
svn_error_dup(svn_error_t *err);

/** Free the memory used by @a error, as well as all ancestors and
 * descendants of @a error.
 *
 * Unlike other Subversion objects, errors are managed explicitly; you
 * MUST clear an error if you are ignoring it, or you are leaking memory.
 * For convenience, @a error may be @c NULL, in which case this function does
 * nothing; thus, svn_error_clear(svn_foo(...)) works as an idiom to
 * ignore errors.
 */
void
svn_error_clear(svn_error_t *error);


/**
 * Very basic default error handler: print out error stack @a error to the
 * stdio stream @a stream, with each error prefixed by @a prefix; quit and
 * clear @a error iff the @a fatal flag is set.  Allocations are performed
 * in the @a error's pool.
 *
 * If you're not sure what prefix to pass, just pass "svn: ".  That's
 * what code that used to call svn_handle_error() and now calls
 * svn_handle_error2() does.
 *
 * @since New in 1.2.
 */
void
svn_handle_error2(svn_error_t *error,
                  FILE *stream,
                  svn_boolean_t fatal,
                  const char *prefix);

/** Like svn_handle_error2() but with @c prefix set to "svn: "
 *
 * @deprecated Provided for backward compatibility with the 1.1 API.
 */
SVN_DEPRECATED
void
svn_handle_error(svn_error_t *error,
                 FILE *stream,
                 svn_boolean_t fatal);

/**
 * Very basic default warning handler: print out the error @a error to the
 * stdio stream @a stream, prefixed by @a prefix.  Allocations are
 * performed in the error's pool.
 *
 * @since New in 1.2.
 */
void
svn_handle_warning2(FILE *stream,
                    svn_error_t *error,
                    const char *prefix);

/** Like svn_handle_warning2() but with @c prefix set to "svn: "
 *
 * @deprecated Provided for backward compatibility with the 1.1 API.
 */
SVN_DEPRECATED
void
svn_handle_warning(FILE *stream,
                   svn_error_t *error);


/** A statement macro for checking error values.
 *
 * Evaluate @a expr.  If it yields an error, return that error from the
 * current function.  Otherwise, continue.
 *
 * The <tt>do { ... } while (0)</tt> wrapper has no semantic effect,
 * but it makes this macro syntactically equivalent to the expression
 * statement it resembles.  Without it, statements like
 *
 * @code
 *   if (a)
 *     SVN_ERR (some operation);
 *   else
 *     foo;
 * @endcode
 *
 * would not mean what they appear to.
 */
#define SVN_ERR(expr)                           \
  do {                                          \
    svn_error_t *svn_err__temp = (expr);        \
    if (svn_err__temp)                          \
      return svn_err__temp;                     \
  } while (0)


/** A statement macro, very similar to @c SVN_ERR.
 *
 * This macro will wrap the error with the specified text before
 * returning the error.
 */
#define SVN_ERR_W(expr, wrap_msg)                           \
  do {                                                      \
    svn_error_t *svn_err__temp = (expr);                    \
    if (svn_err__temp)                                      \
      return svn_error_quick_wrap(svn_err__temp, wrap_msg); \
  } while (0)


/** A statement macro, similar to @c SVN_ERR, but returns an integer.
 *
 * Evaluate @a expr. If it yields an error, handle that error and
 * return @c EXIT_FAILURE.
 */
#define SVN_INT_ERR(expr)                                        \
  do {                                                           \
    svn_error_t *svn_err__temp = (expr);                         \
    if (svn_err__temp) {                                         \
      svn_handle_error2(svn_err__temp, stderr, FALSE, "svn: ");  \
      svn_error_clear(svn_err__temp);                            \
      return EXIT_FAILURE; }                                     \
  } while (0)

/** @} */

/**
 * Return TRUE if @a err is an error specifically related to locking a
 * path in the repository, FALSE otherwise.
 *
 * SVN_ERR_FS_OUT_OF_DATE is in here because it's a non-fatal error
 * that can be thrown when attempting to lock an item.
 *
 * @since New in 1.2.
 */
#define SVN_ERR_IS_LOCK_ERROR(err)                          \
  (err->apr_err == SVN_ERR_FS_PATH_ALREADY_LOCKED ||        \
   err->apr_err == SVN_ERR_FS_OUT_OF_DATE)                  \

/**
 * Return TRUE if @a err is an error specifically related to unlocking
 * a path in the repository, FALSE otherwise.
 *
 * @since New in 1.2.
 */
#define SVN_ERR_IS_UNLOCK_ERROR(err)                        \
  (err->apr_err == SVN_ERR_FS_PATH_NOT_LOCKED ||            \
   err->apr_err == SVN_ERR_FS_BAD_LOCK_TOKEN ||             \
   err->apr_err == SVN_ERR_FS_LOCK_OWNER_MISMATCH ||        \
   err->apr_err == SVN_ERR_FS_NO_SUCH_LOCK ||               \
   err->apr_err == SVN_ERR_RA_NOT_LOCKED ||                 \
   err->apr_err == SVN_ERR_FS_LOCK_EXPIRED)

/** Report that an internal malfunction has occurred, and possibly terminate
 * the program.
 *
 * Act as determined by the current "malfunction handler" which may have
 * been specified by a call to svn_error_set_malfunction_handler() or else
 * is the default handler as specified in that function's documentation. If
 * the malfunction handler returns, then cause the function using this macro
 * to return the error object that it generated.
 *
 * @note The intended use of this macro is where execution reaches a point
 * that cannot possibly be reached unless there is a bug in the program.
 *
 * @since New in 1.6.
 */
#define SVN_ERR_MALFUNCTION()                                   \
  do {                                                          \
    return svn_error__malfunction(__FILE__, __LINE__, NULL);    \
  } while (0)

/** Check that a condition is true: if not, report an error and possibly
 * terminate the program.
 *
 * If the Boolean expression @a expr is true, do nothing. Otherwise,
 * act as determined by the current "malfunction handler" which may have
 * been specified by a call to svn_error_set_malfunction_handler() or else
 * is the default handler as specified in that function's documentation. If
 * the malfunction handler returns, then cause the function using this macro
 * to return the error object that it generated.
 *
 * @note The intended use of this macro is to check a condition that cannot
 * possibly be false unless there is a bug in the program.
 *
 * @note The condition to be checked should not be computationally expensive
 * if it is reached often, as, unlike traditional "assert" statements, the
 * evaluation of this expression is not compiled out in release-mode builds.
 *
 * @since New in 1.6.
 */
#define SVN_ERR_ASSERT(expr)                                \
  do {                                                      \
    if (!(expr))                                            \
      SVN_ERR(svn_error__malfunction(__FILE__, __LINE__, #expr)); \
  } while (0)

/** A helper function for the macros that report malfunctions. Handle a
 * malfunction by calling the current "malfunction handler" which may have
 * been specified by a call to svn_error_set_malfunction_handler() or else
 * is the default handler as specified in that function's documentation.
 *
 * Pass all of the parameters to the handler. The error occurred in the
 * source file @a file at line @a line, and was an assertion failure of the
 * expression @a expr, or, if @a expr is null, an unconditional error.
 *
 * If the malfunction handler returns, then return the error object that it
 * generated.
 *
 * @since New in 1.6.
 */
svn_error_t *
svn_error__malfunction(const char *file,
                       int line,
                       const char *expr);

/** A type of function that handles an assertion failure or other internal
 * malfunction detected within the Subversion libraries.
 *
 * The error occurred in the source file @a file at line @a line, and was an
 * assertion failure of the expression @a expr, or, if @a expr is null, an
 * unconditional error.
 *
 * A function of this type must do one of:
 *   - Return an error object describing the error, using an error code in
 *     the category SVN_ERR_MALFUNC_CATEGORY_START.
 *   - Never return.
 *
 * The function may alter its behaviour according to compile-time
 * and run-time and even interactive conditions.
 *
 * @since New in 1.6.
 */
typedef svn_error_t *(*svn_error_malfunction_handler_t)
  (const char *file, int line, const char *expr);

/** Cause subsequent malfunctions to be handled by @a func.
 * Return the handler that was previously in effect.
 *
 * @a func may not be null.
 *
 * @note The default handler is svn_error_abort_on_malfunction().
 *
 * @note This function must be called in a single-threaded context.
 *
 * @since New in 1.6.
 */
svn_error_malfunction_handler_t
svn_error_set_malfunction_handler(svn_error_malfunction_handler_t func);

/** Handle a malfunction by returning an error object that describes it.
 *
 * This function implements @c svn_error_malfunction_handler_t.
 *
 * @since New in 1.6.
 */
svn_error_t *
svn_error_raise_on_malfunction(const char *file,
                               int line,
                               const char *expr);

/** Handle a malfunction by printing a message to stderr and aborting.
 *
 * This function implements @c svn_error_malfunction_handler_t.
 *
 * @since New in 1.6.
 */
svn_error_t *
svn_error_abort_on_malfunction(const char *file,
                               int line,
                               const char *expr);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SVN_ERROR_H */
