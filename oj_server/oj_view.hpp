#pragma once

#include <iostream>
#include <ctemplate/template.h>

#include "oj_model.hpp"

const std::string template_html = "./template_html/";

namespace ns_view
{
    using namespace ns_model;
    class View
    {
    public:
        void AllExpandHtml(const std::vector<Question>& questions, std::string* html)
        {
            std::string src_html = template_html + "all_questions.html";
            ctemplate::TemplateDictionary root("all_questions");

            for (const auto& q : questions)
            {
                ctemplate::TemplateDictionary* sub = root.AddSectionDictionary("question_list");
                sub->SetValue("number", q.number);
                sub->SetValue("title", q.title);
                sub->SetValue("star", q.star);
            }
            ctemplate::Template* tpl = ctemplate::Template::GetTemplate(src_html, ctemplate::DO_NOT_STRIP);

            tpl->Expand(html, &root);
        }
        void OneExpandHtml(const Question& question, std::string* html)
        {
            std::string src_html = template_html + "one_question.html";
            ctemplate::TemplateDictionary root("one_question");
            root.SetValue("number", question.number);
            root.SetValue("title", question.title);
            root.SetValue("star", question.star);
            root.SetValue("desc", question.desc);
            root.SetValue("pre_code", question.header);

            ctemplate::Template* tpl = ctemplate::Template::GetTemplate(src_html, ctemplate::DO_NOT_STRIP);

            tpl->Expand(html, &root);
        }
    };
}

