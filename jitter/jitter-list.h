/* Jitter: doubly linked list as CPP macros: header.

   Copyright (C) 2018, 2020 Luca Saiu
   Written by Luca Saiu

   This file is part of GNU Jitter.

   GNU Jitter is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   GNU Jitter is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Jitter.  If not, see <http://www.gnu.org/licenses/>. */


#ifndef JITTER_LIST_H_
#define JITTER_LIST_H_

#include <jitter/jitter.h>




/* Doubly linked lists as CPP macros.
 * ************************************************************************** */

/* This is an efficient implementation of a bidirectional linked list data
   structure using CPP macros.

   Linked lists by themselves are not complex, but it's easy to get some detail
   wrong in linking and unlinking operations.  They deserve to be abstracted
   without compromising on efficiency or generality.

   The idea is that the same linking and unlinking macros, independent from
   allocation, can be reused for multiple more complex data structures,
   including some fundamental ones more primitive than heap allocation.

   One use case for this in Jitter is heap allocation and deallocation on
   mmapped executable memory; in the same way, each block of mmapped memory can
   be linked with a predecessor and a successor in a doubly linked list, using
   the memory from the blocks itself for the data structure, without relying on
   malloc.
   if in the future I needed to link hash tables elements to one another to make
   hash iteration faster, I would reuse this.  At some point I would like to run
   Jittery VMs, and epsilon, in kernel mode.  The same heap implementation used
   for mmapped executable memory would work as a malloc replacement as well. */

/* General assumptions:
   - Linked lists are not circular or cyclic in any other way;
   - each item (as per pointer equality) is allowed to be contained in each
     list at most once;
   - each item is allowed to be included in multiple independent linked lists,
     by containing multiple fields of type struct jitter_list_links;
   - each item being linked to a list by one of the macros here is not allowed
     to be already contained in the same list;
   - each item being removed from a list by one of the macros here must be
     contained in the list before the macro expansion is executed;
   - an item being unlinked from a list may be left with invalid "previous"
     and "next" pointers after the macro expansion is executed, or with
     pointers which would conflict with the order established by the items
     remaining in the list;
   - each item being linked or unlinked from a list, passed via pointer,
     is never NULL;
   - memory allocation is *not* handled here: unlinking an item does not
     automatically release its memory. */


/* A struct defining doubly linked list pointers within a list elementx.  This
   struct should be always be contained within another struct called the "item"
   struct, as a named field.
   Link structures ("previous", "next", "first", "last") point to items, and
   not directly to each other.  However, given a pointer to an item, the
   corresponding pointer to its link element can be obtained by adding a
   constant offset known at compile time. */
struct jitter_list_links
{
  /* A pointer to the next item, or NULL for the first object in the list. */
  void *previous;

  /* A pointer to the next item, or NULL for the last object in the list. */
  void *next;
};

/* A struct implementing a doubly linked list header. */
struct jitter_list_header
{
  /* A pointer to the first list item, or NULL if the list is empty.  Equal to
     last and non-NULL only when the list has exactly one element. */
  void *first;

  /* A pointer to the last list item, or NULL if the list is empty.  Equal to
     first and non-NULL only when the list has exactly one element. */
  void *last;
};

/* The following macros all expand to C statements which evaluate their
   expression arguments at most once. */ // FIXME: exactly once?
// FIXME: comment about struct and field names as macro arguments.
// The list is assumed never to contain duplicate (by address) elements, either
// before macros calls or because of their effect.

/* Macros whose names end in _NONEMPTY assume that a list is "always-nonempty".
   A list being "always-nonempty" in this context means that:
   - the list is never empty either before or after the macro calls, and that
   - the first and last items in the list are special elements, not
     changed by the macros.
   In practice every "always-nonempty" list has always at least two elements
   the first and the last, which can't change.
   The "always-nonempty" assumption allows for much more efficient list
   updates using straight-line code, without any conditionals.

   Macros whose name end in _POSSIBLY_AWNE macros are for internal use only.
   Their first argument is always a constant boolean expression, and their
   second argument may be evaluated multiple times. */

/* Expand to a sequence of local variable declarations for links and headers, to
   be used in other macros here.  The expansion is meant as part of a larger
   block, and not protected with do..while (false). */
#define _JITTER_LIST_LOCALS_(item_struct_name, item_field_name, header_p,  \
                             item_p)                                       \
  struct item_struct_name * const _item __attribute__ ((unused))           \
    = (item_p);                                                            \
  struct item_struct_name * const _old_previous __attribute__ ((unused))   \
    = _item->item_field_name.previous;                                     \
  struct item_struct_name * const _old_next __attribute__ ((unused))       \
    = _item->item_field_name.next;                                         \
  struct jitter_list_header * const _header __attribute__ ((unused))       \
    = (header_p);                                                          \
  struct item_struct_name * const _old_first __attribute__ ((unused))      \
    = _header->first;                                                      \
  struct item_struct_name * const _old_last __attribute__ ((unused))       \
    = _header->last;

#define JITTER_LIST_INITIALIZE_HEADER(header_p)                \
  do                                                           \
    {                                                          \
      struct jitter_list_header * const _header = (header_p);  \
      _header->first = NULL;                                   \
      _header->last = NULL;                                    \
    }                                                          \
  while (false)

// Assume that the pointed item does not belong to the list already.
#define JITTER_LIST_LINK_FIRST(item_struct_name, item_field_name, header_p, \
                               item_p)                                      \
  do                                                                        \
    {                                                                       \
      _JITTER_LIST_LOCALS_ (item_struct_name, item_field_name, header_p,    \
                            item_p);                                        \
      _item->item_field_name.previous = NULL;                               \
      _item->item_field_name.next = _old_first;                             \
      if (_old_first != NULL)                                               \
        _old_first->item_field_name.previous = _item;                       \
      if (_old_last == NULL)                                                \
        _header->last = _item;                                              \
      _header->first = _item;                                               \
    }                                                                       \
  while (false)

// Assume that the pointed item does not belong to the list already.
#define JITTER_LIST_LINK_LAST(item_struct_name, item_field_name, header_p,  \
                              item_p)                                       \
  do                                                                        \
    {                                                                       \
      _JITTER_LIST_LOCALS_ (item_struct_name, item_field_name, header_p,    \
                            item_p);                                        \
      _item->item_field_name.previous = _old_last;                          \
      _item->item_field_name.next = NULL;                                   \
      if (_old_last != NULL)                                                \
        _old_last->item_field_name.next = _item;                            \
      if (_old_first == NULL)                                               \
        _header->first = _item;                                             \
      _header->last = _item;                                                \
    }                                                                       \
  while (false)

// Assume that the pointed item does belong to the list.
#define JITTER_LIST_UNLINK_POSSIBLY_AWNE(always_nonempty, item_struct_name,  \
                                         item_field_name, header_p, item_p)  \
  do                                                                         \
    {                                                                        \
      _JITTER_LIST_LOCALS_ (item_struct_name, item_field_name, header_p,     \
                            item_p);                                         \
      if ((always_nonempty) || _old_previous != NULL)                        \
        _old_previous->item_field_name.next = _old_next;                     \
      if ((always_nonempty) || _old_next != NULL)                            \
        _old_next->item_field_name.previous = _old_previous;                 \
      if (! (always_nonempty) && _old_first == _item)                        \
        _header->first = _old_next;                                          \
      if (! (always_nonempty) && _old_last == _item)                         \
        _header->last = _old_previous;                                       \
    }                                                                        \
  while (false)

#define JITTER_LIST_UNLINK(item_struct_name, item_field_name, header_p, item_p) \
  JITTER_LIST_UNLINK_POSSIBLY_AWNE(false, item_struct_name,                     \
                                    item_field_name, header_p, item_p)

/* Same as JITTER_LIST_UNLINK, but assume that the list is always-nonempty.  A
   useful property of JITTER_LIST_UNLINK_NONEMPTY not satisfied by
   JITTER_LIST_UNLINK is that it is possible to unlink a *copy* of an element
   which belongs to the list: item_p is allowed not to belong to the list, as
   long as a copy of it does; in this case the expansion unlinks its copy.
   JITTER_LIST_UNLINK does not allow to do this when the element to be unlinked
   is a copy of the first or the last of the list, which would not compare as
   equal to item_p . */
#define JITTER_LIST_UNLINK_NONEMPTY(item_struct_name, item_field_name,  \
                                    header_p, item_p)                   \
  JITTER_LIST_UNLINK_POSSIBLY_AWNE(true, item_struct_name,              \
                                   item_field_name, header_p, item_p)

// Assume that the pointed item does belong to the list, and that the pointed
// new items does not.
// The old item and the new item *are* allowed to share memory.
#define JITTER_LIST_REPLACE_POSSIBLY_AWNE(always_nonempty, item_struct_name,  \
                                          item_field_name, header_p, item_p,  \
                                          new_item_p)                         \
  do                                                                          \
    {                                                                         \
      _JITTER_LIST_LOCALS_ (item_struct_name, item_field_name, header_p,      \
                            item_p);                                          \
      struct item_struct_name * const _new_item = (new_item_p);               \
      _new_item->item_field_name.previous = _old_previous;                    \
      _new_item->item_field_name.next = _old_next;                            \
      if ((always_nonempty) || _old_previous != NULL)                         \
        _old_previous->item_field_name.next = _new_item;                      \
      if ((always_nonempty) || _old_next != NULL)                             \
        _old_next->item_field_name.previous = _new_item;                      \
      if (! (always_nonempty) && _old_first == _item)                         \
        _header->first = _new_item;                                           \
      if (! (always_nonempty) && _old_last == _item)                          \
        _header->last = _new_item;                                            \
    }                                                                         \
  while (false)

#define JITTER_LIST_REPLACE(item_struct_name, item_field_name, header_p,  \
                            item_p, new_item_p)                           \
  JITTER_LIST_REPLACE_POSSIBLY_AWNE(false, item_struct_name,              \
                                    item_field_name, header_p, item_p,    \
                                    new_item_p)

/* Same as JITTER_LIST_REPLACE, but assume that the list is always-nonempty. */
#define JITTER_LIST_REPLACE_NONEMPTY(allow_empty, item_struct_name,     \
                                    item_field_name, header_p, item_p,  \
                                    new_item_p)                         \
  JITTER_LIST_REPLACE_POSSIBLY_AWNE(true, item_struct_name,             \
                                    item_field_name, header_p, item_p,  \
                                    new_item_p)

#define JITTER_LIST_LINK_BEFORE_POSSIBLY_AWNE(always_nonempty,            \
                                              item_struct_name,           \
                                              item_field_name, header_p,  \
                                              item_p, new_item_p)         \
  do                                                                      \
    {                                                                     \
      _JITTER_LIST_LOCALS_ (item_struct_name, item_field_name, header_p,  \
                            item_p);                                      \
      struct item_struct_name * const _new_item = (new_item_p);           \
      if ((always_nonempty) || _old_previous != NULL)                     \
        _old_previous->item_field_name.next = _new_item;                  \
      _new_item->item_field_name.previous = _old_previous;                \
      _new_item->item_field_name.next = _item;                            \
      _item->item_field_name.previous = _new_item;                        \
      if (! (always_nonempty) && _old_first == _item)                     \
        _header->first = _new_item;                                       \
    }                                                                     \
  while (false)

#define JITTER_LIST_LINK_BEFORE(item_struct_name, item_field_name, header_p,  \
                                item_p, new_item_p)                           \
  JITTER_LIST_LINK_BEFORE_POSSIBLY_AWNE(false, item_struct_name,              \
                                        item_field_name, header_p, item_p,    \
                                        new_item_p)

/* Same as JITTER_LIST_LINK_BEFORE, but assume that the list is
   always-nonempty. */
#define JITTER_LIST_LINK_BEFORE_NONEMPTY(item_struct_name, item_field_name,  \
                                         header_p, item_p, new_item_p)       \
  JITTER_LIST_LINK_BEFORE_POSSIBLY_AWNE(true, item_struct_name,              \
                                        item_field_name, header_p, item_p,   \
                                        new_item_p)


#define JITTER_LIST_LINK_AFTER_POSSIBLY_AWNE(always_nonempty,             \
                                             item_struct_name,            \
                                             item_field_name, header_p,   \
                                             item_p, new_item_p)          \
  do                                                                      \
    {                                                                     \
      _JITTER_LIST_LOCALS_ (item_struct_name, item_field_name, header_p,  \
                            item_p);                                      \
      struct item_struct_name * const _new_item = (new_item_p);           \
      _item->item_field_name.next = _new_item;                            \
      _new_item->item_field_name.previous = _item;                        \
      _new_item->item_field_name.next = _old_next;                        \
      if ((always_nonempty) || _old_next != NULL)                         \
        _old_next->item_field_name.previous = _new_item;                  \
      if (! (always_nonempty) && _old_last == _item)                      \
        _header->last = _new_item;                                        \
    }                                                                     \
  while (false)

#define JITTER_LIST_LINK_AFTER(item_struct_name, item_field_name, header_p,  \
                               item_p, new_item_p)                           \
  JITTER_LIST_LINK_AFTER_POSSIBLY_AWNE(false, item_struct_name,              \
                                       item_field_name, header_p, item_p,    \
                                       new_item_p)

/* Same as JITTER_LIST_LINK_AFTER, but assume that the list is
   always-nonempty. */
#define JITTER_LIST_LINK_AFTER_NONEMPTY(item_struct_name, item_field_name,  \
                                        header_p, item_p, new_item_p)       \
  JITTER_LIST_LINK_AFTER_POSSIBLY_AWNE(true, item_struct_name,              \
                                       item_field_name, header_p, item_p,   \
                                       new_item_p)

/* Expand to a sequence of local variable declarations initialised to the
   header, first and last items, each for one of a "to" and a "from" list; the
   expansion is meant as part of a larger block, and not protected with
   do..while (false). */
#define _JITTER_TWO_LISTS_LOCALS_(item_struct_name, item_field_name,            \
                                  to_header_p, from_header_p)                   \
  struct jitter_list_header * const _jitter_to_header __attribute__ ((unused))  \
    = (to_header_p);                                                            \
  struct item_struct_name * const _jitter_to_first __attribute__ ((unused))     \
    = _jitter_to_header->first;                                                 \
  struct item_struct_name * const _jitter_to_last __attribute__ ((unused))      \
    = _jitter_to_header->last;                                                  \
  struct jitter_list_header * const _jitter_from_header __attribute__ ((unused))\
    = (from_header_p);                                                          \
  struct item_struct_name * const _jitter_from_first __attribute__ ((unused))   \
    = _jitter_from_header->first;                                               \
  struct item_struct_name * const _jitter_from_last __attribute__ ((unused))    \
    = _jitter_from_header->last;

/* Make the from list empty, moving all of its elements to the beginning
   of the to list. */
#define JITTER_LIST_PREPEND_LIST(item_struct_name, item_field_name,      \
                                 to_header_p, from_header_p)             \
  do                                                                     \
    {                                                                    \
      _JITTER_TWO_LISTS_LOCALS_ (item_struct_name, item_field_name,      \
                                 (to_header_p), (from_header_p));        \
      /* At the end from is empty.  The final configuration of to is:    \
         from_first .. from_last , to_first .. to_last */                \
      struct item_struct_name * _jitter_new_first = _jitter_from_first;  \
      if (_jitter_new_first == NULL)                                     \
        _jitter_new_first = _jitter_to_first;                            \
      struct item_struct_name * _jitter_new_last = _jitter_to_last;      \
      if (_jitter_new_last == NULL)                                      \
        _jitter_new_last = _jitter_from_last;                            \
      if (_jitter_from_last != NULL)                                     \
        _jitter_from_last->item_field_name.next = _jitter_to_first;      \
      if (_jitter_to_first != NULL)                                      \
        _jitter_to_first->item_field_name.previous = _jitter_from_last;  \
      _jitter_to_header->first = _jitter_new_first;                      \
      _jitter_to_header->last = _jitter_new_last;                        \
      _jitter_from_header->first = NULL;                                 \
      _jitter_from_header->last = NULL;                                  \
    }                                                                    \
  while (false)

/* Make the from list empty, moving all of its elements to the end of the
   to list. */
#define JITTER_LIST_APPEND_LIST(item_struct_name, item_field_name,       \
                                to_header_p, from_header_p)              \
  do                                                                     \
    {                                                                    \
      _JITTER_TWO_LISTS_LOCALS_ (item_struct_name, item_field_name,      \
                                 (to_header_p), (from_header_p));        \
      /* At the end from is empty.  The final configuration of to is:    \
         to_first .. to_last , from_first .. from_last */                \
      struct item_struct_name * _jitter_new_first = _jitter_to_first;    \
      if (_jitter_new_first == NULL)                                     \
        _jitter_new_first = _jitter_from_first;                          \
      struct item_struct_name * _jitter_new_last = _jitter_from_last;    \
      if (_jitter_new_last == NULL)                                      \
        _jitter_new_last = _jitter_to_last;                              \
      if (_jitter_to_last != NULL)                                       \
        _jitter_to_last->item_field_name.next = _jitter_from_first;      \
      if (_jitter_from_first != NULL)                                    \
        _jitter_from_first->item_field_name.previous = _jitter_to_last;  \
      _jitter_to_header->first = _jitter_new_first;                      \
      _jitter_to_header->last = _jitter_new_last;                        \
      _jitter_from_header->first = NULL;                                 \
      _jitter_from_header->last = NULL;                                  \
    }                                                                    \
  while (false)

#endif // #ifndef JITTER_LIST_H_
