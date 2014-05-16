/* class.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

/*

Copyright (c) 2005-2014 Made to Order Software Corp.

http://snapwebsites.org/project/as2js

Permission is hereby granted, free of charge, to any
person obtaining a copy of this software and
associated documentation files (the "Software"), to
deal in the Software without restriction, including
without limitation the rights to use, copy, modify,
merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the
following conditions:

The above copyright notice and this permission notice
shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include    "as2js/parser.h"
#include    "as2js/message.h"


namespace as2js
{


/**********************************************************************/
/**********************************************************************/
/***  PARSER CLASS  ***************************************************/
/**********************************************************************/
/**********************************************************************/

void Parser::class_declaration(Node::pointer_t& node, Node::node_t type)
{
    if(f_node->get_type() != Node::NODE_IDENTIFIER)
    {
        Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INVALID_CLASS, f_lexer->get_input()->get_position());
        msg << "the name of the class is expected after the keyword 'class'";
        return;
    }

    // *** NAME ***
    node = f_lexer->get_new_node(type);
    node->set_string(f_node->get_string());

    // *** INHERITANCE ***
    get_token();
    while(f_node->get_type() == Node::NODE_EXTENDS
       || f_node->get_type() == Node::NODE_IMPLEMENTS)
    {
        Node::pointer_t inherits(f_node);
        node->append_child(inherits);

        get_token();

        Node::pointer_t expr;
        expression(expr);
        if(expr)
        {
            inherits->append_child(expr);
        }
        else
        {
            // TBD: we may not need this error since the expression() should
            //      already generate an error as required
            Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INVALID_CLASS, f_lexer->get_input()->get_position());
            msg << "expected a valid expression after '" << f_node->get_type_name() << "'";
            //return; -- continue? -- it was before...
        }
        // TODO: EXTENDS and IMPLEMENTS don't accept assignments.
        // TODO: EXTENDS doesn't accept lists.
        //     We need to test for that here.
    }
    // TODO: note that we only can accept one EXTENDS and
    //     one IMPLEMENTS in that order. We need to check
    //     that here. [that's according to the AS spec. is
    //     that really important?]

    if(f_node->get_type() == Node::NODE_OPEN_CURVLY_BRACKET)
    {
        get_token();

        // *** DECLARATION ***
        if(f_node->get_type() != Node::NODE_CLOSE_CURVLY_BRACKET)
        {
            Node::pointer_t directive_list_node(f_lexer->get_new_node(Node::NODE_DIRECTIVE_LIST));
            node->append_child(directive_list_node);
        }

        if(f_node->get_type() == Node::NODE_CLOSE_CURVLY_BRACKET)
        {
            get_token();
        }
        else
        {
            Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_CURVLY_BRAKETS_EXPECTED, f_lexer->get_input()->get_position());
            msg << "'}' expected to close the 'class' definition";
        }
    }
    else if(f_node->get_type() != Node::NODE_SEMICOLON)
    {
        // accept empty class definitions (for typedef's and forward declaration)
        Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_CURVLY_BRAKETS_EXPECTED, f_lexer->get_input()->get_position());
        msg << "'{' expected to start the 'class' definition";
    }
}




/**********************************************************************/
/**********************************************************************/
/***  PARSER ENUM  ****************************************************/
/**********************************************************************/
/**********************************************************************/

void Parser::enum_declaration(Node::pointer_t& node)
{
    node = f_lexer->get_new_node(Node::NODE_ENUM);

    // enumerations can be unamed
    if(f_node->get_type() == Node::NODE_IDENTIFIER)
    {
        node->set_string(f_node->get_string());
        get_token();
    }

    // in case the name was not specified, we can still have a type (?)
    if(f_node->get_type() == Node::NODE_COLON)
    {
        Node::pointer_t type;
        expression(type);
        node->append_child(type);
    }

    if(f_node->get_type() != Node::NODE_OPEN_CURVLY_BRACKET)
    {
        if(f_node->get_type() == Node::NODE_SEMICOLON)
        {
            // empty enumeration (i.e. forward declaration)
            return;
        }
        Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_CURVLY_BRAKETS_EXPECTED, f_lexer->get_input()->get_position());
        msg << "'{' expected to start the 'enum' definition";
        return;
    }

    get_token();

    Node::pointer_t previous(f_lexer->get_new_node(Node::NODE_NULL));
    while(f_node->get_type() != Node::NODE_CLOSE_CURVLY_BRACKET
       && f_node->get_type() != Node::NODE_EOF)
    {
        if(f_node->get_type() == Node::NODE_COMMA)
        {
            // skip to the next token
            get_token();

            Message msg(MESSAGE_LEVEL_WARNING, AS_ERR_UNEXPECTED_PUNCTUATION, f_lexer->get_input()->get_position());
            msg << "',' unexpected without a name";
            continue;
        }
        String current_name("null");
        Node::pointer_t entry(f_lexer->get_new_node(Node::NODE_VARIABLE));
        node->append_child(entry);
        if(f_node->get_type() == Node::NODE_IDENTIFIER)
        {
            entry->set_flag(Node::NODE_VAR_FLAG_CONST, true);
            entry->set_flag(Node::NODE_VAR_FLAG_ENUM, true);
            current_name = f_node->get_string();
            get_token();
        }
        else
        {
            Message msg(MESSAGE_LEVEL_WARNING, AS_ERR_INVALID_ENUM, f_lexer->get_input()->get_position());
            msg << "each 'enum' entry needs to include an identifier";
        }
        Node::pointer_t expr;
        if(f_node->get_type() == Node::NODE_ASSIGNMENT)
        {
            get_token();
            conditional_expression(expr, false);
        }
        else if(previous->get_type() == Node::NODE_NULL)
        {
            // very first time
            expr = f_lexer->get_new_node(Node::NODE_INT64);
            //expr->set_int64(0); -- this is the default
        }
        else
        {
            expr = f_lexer->get_new_node(Node::NODE_ADD);
            expr->append_child(previous); // left handside
            Node::pointer_t one(f_lexer->get_new_node(Node::NODE_INT64));
            one->set_int64(1);
            expr->append_child(one);
        }

        Node::pointer_t set(f_lexer->get_new_node(Node::NODE_SET));
        set->append_child(expr);
        entry->append_child(set);

        previous = f_lexer->get_new_node(Node::NODE_IDENTIFIER);
        previous->set_string(current_name);

        if(f_node->get_type() == Node::NODE_NULL)
        {
            get_token();
        }
        else if(f_node->get_type() == Node::NODE_CLOSE_CURVLY_BRACKET)
        {
            Message msg(MESSAGE_LEVEL_WARNING, AS_ERR_UNEXPECTED_PUNCTUATION, f_lexer->get_input()->get_position());
            msg << "',' expected between enumeration elements";
        }
    }

    if(f_node->get_type() == Node::NODE_CLOSE_CURVLY_BRACKET)
    {
        get_token();
    }
    else
    {
        Message msg(MESSAGE_LEVEL_WARNING, AS_ERR_CURVLY_BRAKETS_EXPECTED, f_lexer->get_input()->get_position());
        msg << "'}' expected to close the 'enum' definition";
    }
}




}
// namespace as

// vim: ts=4 sw=4 et
