#include <nitro.h>
#include <nitro/utest.h>

UTiResult UTi_Result;

void UTi_Main (int num_module, UTiModule * modules)
{
    int i;
    int result = 0;
    UTiModule * t = modules;

    for (i = 0; i < num_module; i++) {
        UTi_Printf("%s\n", t->testname);
        UTi_SetResult(UT_RESULT_UNTESTED);
        UTi_ClearMessage();

        if (t->callback_begin) {
            t->callback_begin();
        }

        if (t->callback) {
            t->callback();
        }

        if (t->callback_end) {
            t->callback_end();
        }

        switch (UTi_GetResult()) {
        case UT_RESULT_OK:
            UTi_PutString("--->OK\n\n");
            break;
        case UT_RESULT_NG:
            UTi_PutString(UTi_GetMessage());
            UTi_PutString("--->NG\n\n");
            result = 1;
            break;
        case UT_RESULT_UNTESTED:
            UTi_PutString("--->??\n\n");
            break;
        default:
            OS_Panic("System Error!!");
            break;
        }

        t++;
    }
    UTi_Printf("%c%c", -1, result + '0');
}
