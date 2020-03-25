#include <windows.h>
#include <stdio.h>
#include <string>
#include <utility>


#define NUMBER_OF(x)    (sizeof(x) / sizeof((x)[0]))

#define DEFAULT_TIMEOUT 5000


struct Test
{
    const char* name;

    // One of TEST_XXX flags
    unsigned type;

    // Combination of TEST_XXX flags listing the tests that can be combined
    // with this one.
    unsigned mask;

    // Should the test pass of fail?
    bool positive;

    // Prepare and run routine
    bool (*tearUp)();

    // Cleanup routine
    void (*tearDown)();
};

typedef std::pair<const Test*, const Test*> TestVariants;


void
run(
    const TestVariants* first, 
    const TestVariants* last,
    std::string name = std::string(),
    unsigned mask = 0,
    bool positive = true
    )
{
    if (first != last)
    {
        for (const Test* i = first->first; i != first->second; ++i)
        {
            if (
                ((mask & i->type) == 0) &&
                ((mask & ~(i->type | i->mask)) == 0))
            {
                std::string fullName = 
                    name.empty() ? std::string(i->name) : name + ", " + i->name;

                try
                {
                    bool result = i->tearUp();

                    if (result)
                    {
                        run(
                            first + 1, 
                            last, 
                            fullName,
                            mask | i->type, 
                            positive & i->positive);
                    }
                    else
                    {
                        if (positive & i->positive)
                        {
                            printf("%s FAILED (positive)\n", fullName.c_str());
                        }
                        else
                        {
                            printf("%s PASSED\n", fullName.c_str());
                        }
                    }

                    i->tearDown();
                }
                catch (const std::exception&)
                {
                    printf("%s FAILED (exception)\n", fullName.c_str());
                }
            }
        }
    }
    else
    {
        if (positive)
        {
            printf("%s PASSED\n", name.c_str());
        }
        else
        {
            printf("%s FAILED (negative)\n", name.c_str());
        }
    }
}


//
// Test data
//

static HANDLE thread = NULL;

static DWORD threadId = 0;
static BOOL inherit = FALSE;
static DWORD accessMask = 0;


//
// Test functions
//

DWORD WINAPI 
ThreadProc(
    __in LPVOID parameter
    )
{
    UNREFERENCED_PARAMETER(parameter);
    return 0;
}


bool
createThreadSelf()
{
    if (thread != NULL)
    {
        fprintf(
            stderr, 
            __FUNCTION__ ": Cleanup has not been done correctly\n");
        return false;
    }

    BOOL res = 
        DuplicateHandle(
            GetCurrentProcess(),
            GetCurrentThread(),
            GetCurrentProcess(),
            &thread,
            THREAD_ALL_ACCESS,
            FALSE,
            0);
    
    if (!res)
    {
        fprintf(
            stderr, 
            __FUNCTION__ ": DuplicateHandle failed: %08x\n", 
            GetLastError());
        return false;
    }

    threadId = GetCurrentThreadId();

    return true;
}


void
shutdownThreadSelf()
{
    if (thread != NULL)
    {
        CloseHandle(thread);

        thread = NULL;
        threadId = 0;
    }
}


bool
createThreadExisting()
{
    if (thread != NULL)
    {
        fprintf(
            stderr, 
            __FUNCTION__ ": Cleanup has not been done correctly\n");
        return false;
    }

    thread =
        CreateThread(
            NULL,
            0,
            ThreadProc,
            0,
            CREATE_SUSPENDED,
            &threadId);

    if (thread == NULL)
    {
        fprintf(
            stderr, 
            __FUNCTION__ ": CreateThread failed: %08x\n", 
            GetLastError());
        return false;
    }

    return true;
}


void
shutdownThreadExisting()
{
    if (thread != NULL)
    {
        ResumeThread(thread);
        WaitForSingleObject(thread, DEFAULT_TIMEOUT);
        CloseHandle(thread);

        thread = NULL;
        threadId = 0;
    }
}


bool
createThreadNotExisting()
{
    if (thread != NULL)
    {
        fprintf(
            stderr, 
            __FUNCTION__ ": Cleanup has not been done correctly\n");
        return false;
    }

    thread = NULL;
    threadId = 0xffffff00;

    return true;
}


void
shutdownThreadNotExisting()
{
    thread = NULL;
    threadId = 0;
}


bool
setInheritTrue()
{
    inherit = TRUE;
    return true;
}


bool
setInheritFalse()
{
    inherit = FALSE;
    return true;
}


bool
setValidAccessMask()
{
    accessMask = THREAD_ALL_ACCESS;
    return true;
}


bool
setInvalidAccessMask()
{
    accessMask = THREAD_ALL_ACCESS | (1 << 25);
    return true;
}


void
doNothing()
{}


bool
callOpenThread()
{
    HANDLE h = 
        OpenThread(
            accessMask,
            inherit,
            threadId);

    if (!h)
    {
        return false;
    }


    DWORD flags = 0;
    BOOL res = GetHandleInformation(h, &flags);

    if (!res)
    {
        return false;
    }

    return inherit == ((flags & HANDLE_FLAG_INHERIT) == HANDLE_FLAG_INHERIT);
}


//
// Tests registration
//

enum
{
    TEST_THREAD_SELF            = 1,
    TEST_THREAD_EXISTING        = TEST_THREAD_SELF << 1,
    TEST_THREAD_NOT_EXISTING    = TEST_THREAD_EXISTING << 1,
    TEST_THREAD_ALL             = 
        TEST_THREAD_SELF | TEST_THREAD_EXISTING | TEST_THREAD_NOT_EXISTING,
    
    TEST_INHERIT_TRUE           = TEST_THREAD_NOT_EXISTING << 1,
    TEST_INHERIT_FALSE          = TEST_INHERIT_TRUE << 1,
    TEST_INHERIT_ALL            = 
        TEST_INHERIT_TRUE | TEST_INHERIT_FALSE,
    
    TEST_ACCESS_VALID           = TEST_INHERIT_FALSE << 1,
    TEST_ACCESS_INVALID         = TEST_ACCESS_VALID << 1,
    TEST_ACCESS_ALL             = 
        TEST_ACCESS_VALID | TEST_ACCESS_INVALID,

    TEST_CALL_OPEN_THREAD       = TEST_ACCESS_INVALID << 1
};


static Test testsThread[] =
{
    {
        "Self",
        TEST_THREAD_SELF,
        TEST_INHERIT_ALL | TEST_ACCESS_ALL | TEST_CALL_OPEN_THREAD,
        TRUE,
        createThreadSelf,
        shutdownThreadSelf
    },
    {
        "Existing thread",
        TEST_THREAD_EXISTING,
        TEST_INHERIT_ALL | TEST_ACCESS_ALL | TEST_CALL_OPEN_THREAD,
        TRUE,
        createThreadExisting,
        shutdownThreadExisting
    },
    {
        "Not existing thread",
        TEST_THREAD_NOT_EXISTING,
        TEST_INHERIT_ALL | TEST_ACCESS_ALL | TEST_CALL_OPEN_THREAD,
        FALSE,
        createThreadNotExisting,
        shutdownThreadNotExisting
    }
};
    

static Test testsInherit[] =
{
    {
        "Inherit",
        TEST_INHERIT_TRUE,
        TEST_THREAD_ALL | TEST_ACCESS_ALL | TEST_CALL_OPEN_THREAD,
        TRUE,
        setInheritTrue,
        doNothing
    },
    {
        "Not inherit",
        TEST_INHERIT_FALSE,
        TEST_THREAD_ALL | TEST_ACCESS_ALL | TEST_CALL_OPEN_THREAD,
        TRUE,
        setInheritFalse,
        doNothing
    }
};

    
static Test testsAccess[] =
{
    {
        "Valid access mask",
        TEST_ACCESS_VALID,
        TEST_THREAD_ALL | TEST_INHERIT_ALL | TEST_CALL_OPEN_THREAD,
        TRUE,
        setValidAccessMask,
        doNothing
    },
    {
        "Invalid access mask",
        TEST_ACCESS_INVALID,
        TEST_THREAD_ALL | TEST_INHERIT_ALL | TEST_CALL_OPEN_THREAD,
        TRUE,
        setInvalidAccessMask,
        doNothing
    }
};


static Test testCallOpenThread =
{
    "OpenThread",
    TEST_CALL_OPEN_THREAD,
    TEST_THREAD_ALL | TEST_INHERIT_ALL | TEST_ACCESS_ALL,
    TRUE,
    callOpenThread,
    doNothing
};


static TestVariants tests[] = 
{
    TestVariants(testsThread,           testsThread   + NUMBER_OF(testsThread)),
    TestVariants(testsInherit,          testsInherit  + NUMBER_OF(testsInherit)),
    TestVariants(testsAccess,           testsAccess   + NUMBER_OF(testsAccess)),
    TestVariants(&testCallOpenThread,   &testCallOpenThread + 1)
};


int 
__cdecl
main(
    int argc,
    const char* argv[]
    )
{
    run(tests, tests + NUMBER_OF(tests));
    return 0;
}

