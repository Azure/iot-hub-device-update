#ifndef WORKFLOW_PTR_HPP
#define WORKFLOW_PTR_HPP

#include <memory> // std::unique_ptr

#include <aduc/workflow_utils.h> // workflow_free_string, workflow_free_file_entity

namespace ptr_deleters
{
struct workflow_string
{
    void operator()(char* ptr)
    {
        workflow_free_string(ptr);
    }
};
struct workflow_file_entity
{
    void operator()(ADUC_FileEntity* ptr)
    {
        workflow_free_file_entity(ptr);
    }
};
} // namespace ptr_deleters

using workflow_string_ptr = std::unique_ptr<char, ptr_deleters::workflow_string>;

using workflow_file_entity_ptr = std::unique_ptr<ADUC_FileEntity, ptr_deleters::workflow_file_entity>;

#endif
