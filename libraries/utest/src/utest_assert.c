#include <nitro.h>
#include <nitro/utest.h>

void UTi_Assert (const char * file, int line, const char * exp_str, int exp_val)
{
    if (!exp_val) {
        UTi_SetResult(UT_RESULT_NG);
        UTi_SetMessage(file, line, "UT_Assert: %s[=%d] is not TRUE\n", exp_str, exp_val);
    } else if (UTi_GetResult() != UT_RESULT_NG)   {
        UTi_SetResult(UT_RESULT_OK);
    }

    return;
}

void UTi_AssertEq (const char * file, int line, const char * exp_str1, int exp_val1, const char * exp_str2, int exp_val2)
{
    if (exp_val1 != exp_val2) {
        UTi_SetResult(UT_RESULT_NG);
        UTi_SetMessage(file, line, "UT_AssertEq: %s[=%d] is not equal to %s[=%d]\n", exp_str1,
                       exp_val1, exp_str2, exp_val2);
    } else if (UTi_GetResult() != UT_RESULT_NG)   {
        UTi_SetResult(UT_RESULT_OK);
    }
    return;
}

void UTi_AssertNe (const char * file, int line, const char * exp_str1, int exp_val1, const char * exp_str2, int exp_val2)
{

    if (exp_val1 == exp_val2) {
        UTi_SetResult(UT_RESULT_NG);
        UTi_SetMessage(file, line, "UT_AssertNe: %s[=%d] is equal to %s[=%d]\n", exp_str1, exp_val1, exp_str2, exp_val2);
    } else if (UTi_GetResult() != UT_RESULT_NG)   {
        UTi_SetResult(UT_RESULT_OK);
    }

    return;
}

void UTi_AssertMemEq (const char * file, int line, const char * exp_str1, const void * exp_ptr1, const char * exp_str2, const void * exp_ptr2, int exp_len)
{
    if (UTi_MemCmp(exp_ptr1, exp_ptr2, exp_len)) {
        UTi_SetResult(UT_RESULT_NG);
        UTi_SetMessage(file, line, "UT_AssertMemEq: %s is not equal to %s (size=%d)\n", exp_str1,
                       exp_str2, exp_len);
    } else if (UTi_GetResult() != UT_RESULT_NG) {
        UTi_SetResult(UT_RESULT_OK);
    }

    return;
}

void UTi_AssertMemNe (const char * file, int line, const char * exp_str1, const void * exp_ptr1, const char * exp_str2, const void * exp_ptr2, int exp_len)
{
    if (!UTi_MemCmp(exp_ptr1, exp_ptr2, exp_len)) {
        UTi_SetResult(UT_RESULT_NG);
        UTi_SetMessage(file, line, "UT_AssertMemNe: %s is equal to %s (size=%d)\n", exp_str1,
                       exp_str2, exp_len);
    } else if (UTi_GetResult() != UT_RESULT_NG)   {
        UTi_SetResult(UT_RESULT_OK);
    }

    return;
}

OSContext UTi_AssertContext;

#ifdef SDK_DEBUG
    void UTi_Terminate (void)
    {
        OS_LoadContext(&UTi_AssertContext);
    }
#endif

void UTi_AssertAsserted (const char * file, int line, const char * exp_str, int asserted)
{
    if (!asserted) {
        UTi_SetResult(UT_RESULT_NG);
        UTi_SetMessage(file, line, "UT_AssertAsserted: %s not asserted\n", exp_str);
    } else if (UTi_GetResult() != UT_RESULT_NG)   {
        UTi_SetResult(UT_RESULT_OK);
    }

    return;
}

void UTi_AssertNotAsserted (const char * file, int line, const char * exp_str, int asserted)
{
    if (asserted) {
        UTi_SetResult(UT_RESULT_NG);
        UTi_SetMessage(file, line, "UT_AssertNotAsserted: %s asserted\n", exp_str);
    } else if (UTi_GetResult() != UT_RESULT_NG)   {
        UTi_SetResult(UT_RESULT_OK);
    }

    return;
}

int UTi_MemCmp (const void * ptr1, const void * ptr2, int len)
{
    int i;

    for (i = 0; i < len; i++) {
        if (((unsigned char *)ptr1)[i] != ((unsigned char *)ptr2)[i]) {
            return 1;
        }
    }

    return 0;
}