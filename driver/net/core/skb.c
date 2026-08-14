#include <delay.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include <net/net.h>
#include <net/skb.h>

// FIXME!
struct sock_buff *skb_alloc(__u32 prot_len, __u32 data_len)
{
	struct sock_buff *skb;

	skb = malloc(sizeof(struct sock_buff));
	if (NULL == skb)
		return NULL;

	skb->head = malloc((prot_len + data_len + 1) & ~1);
	if (NULL == skb->head) {
		DPRINT("%s(): malloc failed (size = %d bytes)!\n",
			__func__, prot_len + data_len);
		return NULL;
	}

	skb->data = skb->head + prot_len;
	skb->size = data_len;

	INIT_LIST_HEAD(&skb->node);

	// FIXME!!
	skb->ndev = ndev_get_first();
	assert(skb->ndev);

	return skb;
}

void skb_free(struct sock_buff *skb)
{
	assert(skb && skb->head);

	free(skb->head);
	free(skb);
}
