/**
* A simple file to validate your automated test setup for AESD
*/

#include "autotest-validate.h"
#include <stdbool.h>

/**
* @return true (as you may have guessed from the name)
*/
bool this_function_returns_true()
{
    return true;
}

/**
* @return false (as you may have guessed from the name)
*/
bool this_function_returns_false()
{
    return false;
}

/**
 * @return a string which contains the username you use for
 * git submissions.  This string should match the string in conf/username.txt
 */
const char* my_username()
{
    FILE* fptr;
    fptr = fopen("conf/username.txt", "r");
    const int content_size = 100;
    char* file_content = malloc(content_size);
    fgets(file_content, content_size, fptr);
    fclose(fptr);
    return file_content;
}
