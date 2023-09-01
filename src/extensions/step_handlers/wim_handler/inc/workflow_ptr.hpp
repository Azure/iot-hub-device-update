#ifndef WORKFLOW_PTR_HPP
#define WORKFLOW_PTR_HPP

#include <memory> // std::unique_ptr

#include <aduc/workflow_utils.h> // workflow_free_string

namespace ptr_deleters
{
struct workflow_string // !< wrapper for the workflow struct so we can keep free the workflow_string ptr
{
    /**
     * @brief Frees the @p ptr parameter using workflow_free_string()
     * @param ptr the workflow string to be freed 
     */
    void operator()(char* ptr)
    {
        workflow_free_string(ptr);
    }
};

} //namespace ptr_deleters

using workflow_string_ptr = std::unique_ptr<char, ptr_deleters::workflow_string>;

#endif
