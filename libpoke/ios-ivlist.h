

struct NODE_IMPL
{
  struct NODE_IMPL *prev;
  struct NODE_IMPL *next;
  uint64_t low;
  uint64_t high;

  NODE_PAYLOAD_FIELDS
};

typedef struct NODE_IMPL * NODE_T;

static NODE_T
interval_mknode (uint64_t low, uint64_t high, NODE_PAYLOAD_PARAMS)
{
  NODE_T new_node =
    (struct NODE_IMPL *) malloc (sizeof (struct NODE_IMPL));

  if (!new_node)
    return NULL;

  new_node->prev = NULL;
  new_node->next = NULL;
  new_node->low = low;
  new_node->high = high;

  NODE_PAYLOAD_ASSIGN (new_node);
  return new_node;
}

static int
ios_ivlist_insert (CONTAINER_T container, uint64_t low, uint64_t high,
		   NODE_PAYLOAD_PARAMS)
{

  NODE_T new_node = interval_mknode (low, high, NODE_PAYLOAD_ARGS);
  if (!new_node)
    return IOS_ENOMEM;

  if (!container->root)
    container->root = new_node;
  else
    {
      container->tail->next = new_node;
      new_node->prev = container->tail;
    }

  container->tail = new_node;
  container->count++;
  return IOS_OK;
}

static int
ios_ivlist_remove (CONTAINER_T container, NODE_T node)
{
  if (!node)
    return IOS_EINVAL;

  if (node->prev)
    node->prev->next = node->next;
  if (node->next)
    node->next->prev = node->prev;

  if (container->root == node)
    container->root = node->next;

  container->count--;
  return IOS_OK;
}

static NODE_T
ios_ivlist_lookup (CONTAINER_T container, NODE_PAYLOAD_PARAMS)
{
  for (NODE_T node = container->root; node != NULL; node = node->next)
    {
      if (NODE_PAYLOAD_EQUALS (node))
	return node;
    }

  return NULL;
}

static void
ios_ivlist_visit_all (CONTAINER_T container, NODE_VISITOR_FN fn)
{
  for (NODE_T node = container->root; node != NULL; node = node->next)
    fn (NODE_PAYLOAD_ACCESS (node));
}

static bool
overlaps (NODE_T node, uint64_t low, uint64_t high)
{
  return (node->low <= high && node->high >= low);
}

static void
ios_ivlist_visit_overlaps (CONTAINER_T container, uint64_t begin, uint64_t end,
			   NODE_VISITOR_FN fn)
{
  for (NODE_T node = container->root; node != NULL; node = node->next)
    {
      if (overlaps (node, begin, end))
	fn (NODE_PAYLOAD_ACCESS (node));
    }
}

static void
ios_ivlist_destroy (CONTAINER_T container)
{
  for (NODE_T node = container->root; node != NULL ;)
    {
      NODE_T next = node->next;
      free (node);
      node = next;
    }
  container->root = NULL;
  container->count = 0;
}
