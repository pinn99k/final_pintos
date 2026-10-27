/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "vaddr.h"

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void
vm_init (struct hash *vm) {
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* For project 4 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type (struct page *page) {
	int ty = VM_TYPE (page->operations->type);
	switch (ty) {
		case VM_UNINIT:
			return VM_TYPE (page->uninit.type);
		default:
			return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);

/* Returns a hash value for page p. */
unsigned
page_hash (const struct hash_elem *p_, void *aux UNUSED) {
  const struct page *p = hash_entry (p_, struct page, hash_elem);
  return hash_bytes (&p->va, sizeof p->va);
}

/* Returns true if page a precedes page b. */
bool
page_less (const struct hash_elem *a_,
           const struct hash_elem *b_, void *aux UNUSED) {

	if (a_ == NULL){
		return true;
	}
	if (b_ == NULL){
		return false;
	}
	const struct page *a = hash_entry (a_, struct page, hash_elem);
	const struct page *b = hash_entry (b_, struct page, hash_elem);

	return a->va < b->va;
}
/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {

	ASSERT (VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current ()->spt;

	struct page *page = NULL;
	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page (spt, upage) == NULL) {
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */


		/* spt_destory or kill 해줄때 page도 free 해줘야함. (메모리누수) */
		page = malloc (sizeof (struct page));

		/* 예외 처리 */
		if (page == NULL){
			goto err;
		}

		/* lazy_load 콜백함수 추가, 페이지 타입별 초기화 함수 지정 */
		switch (VM_TYPE (type))
		{
		case VM_ANON :
			uninit_new (page, upage, init, type, aux, anon_initializer);
			break;		
		case VM_FILE :	
			uninit_new (page, upage, init, type, aux, file_backed_initializer);
			break;
		default:
			free(page);
			goto err;
			break;
		}

		/* TODO: Insert the page into the spt. */
		if (!spt_insert_page(spt, upage)){
			free(page);
			goto err;
		}
		return true;
	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
	struct page *page = NULL;
	
	/* TODO: Fill this function. */
	page = (struct page *)malloc(sizeof (struct page));
	// 페이지와 연결
	page->va = pg_round_down(va);
	
	/* 못찾으면 NULL 리턴 */
	struct hash_elem *e = hash_find(&spt->pages, &page->hash_elem);
	free(page);

	// 있으면 e에 해당하는 페이지 반환
	return e != NULL ? hash_entry(e, struct page, hash_elem) : NULL;
}

/* Insert PAGE into spt with validation. */
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	/* 예외 처리 */
	if(spt == NULL || page == NULL){
		return false;
	}

	/* 내장 함수 사용 */
	int succ = hash_insert(&spt->pages, &page->hash_elem);

	return succ;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	vm_dealloc_page (page);
	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	 /* TODO: The policy for eviction is up to you. */

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */

	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame (void) {
	struct frame *frame = NULL;
	/* TODO: Fill this function. */

	/* 
	말록 해서 프레임 공간 가져오면 되는거 아님? 
	KVA를 가져와서 넣어줘야 하는거 아닌가?
	*/

	frame = malloc(sizeof(struct frame));
	// 어디서 초기화 하지?
	// 프레임 지울때? 스왑할때 빼야할 듯

	frame->kva = palloc_get_page(PAL_USER | PAL_ZERO);
	if(frame == NULL){
		//페이지 스왑
		return NULL;
	}
	
	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);  
	
	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
	// addr을 기준으로 PGSIZE만큼 확장하면 되나?
	vm_alloc_page(VM_ANON, addr, true);
	vm_claim_page(addr);
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	struct supplemental_page_table *spt UNUSED = &thread_current ()->spt;

	/* TODO: Validate the fault */
	/* TODO: Your code goes here */

	/* 권한위반 검사 */
	if (addr == NULL || !is_user_vaddr(addr) || !not_present){
		return false;
	}
	// !pml4_get_page(thread_current()->pml4, addr))

	if(!write){
		// 둘 중 뭔지 모르겠음
		return false;
	}

	// spt에 페이지가 존재하는지 찾기
	struct page *page = spt_find_page(spt, addr);

	if(page == NULL){
		/* TODO 
		페이지가 NULL 이면 stack 이 자랄 수 있는지 찾아서 늘리는게 아닌가?

		늘리는거
		userstack과 최대크기 사이에 존재하면서 rsp 보다 높은 주소에 있을때
		*/

		// rsp 검사
		// 폴트가 스택 확장으로 처리할 수 있는지 검사
		addr = pg_round_down(addr);
		void *rsp = thread_current()->user_rsp;

		// userstack과 최대크기 사이에 존재하면서 rsp 보다 높은 주소에 있을때
		// Pintos에서는 최대 스택 크기를 1MB(1,048,576 bytes)로 제한
		if (addr >= rsp - PGSIZE && USER_STACK - (1<<20) <= addr < USER_STACK){
			vm_stack_growth(addr);
			return true;
		}
	}
	// 스택 검사도 구현해야 하는 걸로 알고있음
	// 근데 모르겠어요... 일단 패스

	return vm_do_claim_page (page);
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* Claim the page that allocate on VA. */
bool
vm_claim_page (void *va UNUSED) {
	struct page *page = NULL;

	/* TODO: Fill this function */
	// spt에 페이지가 있나 확인하기
	page = spt_find_page(&thread_current ()->spt, va);

	// 없으면 실패 반환?
	if(page ==NULL){
		return false;
	}

	// 있으면 요청하기 뭔가 처리해 
	// 스왑을 하던 할당을 하던
	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();
	// NULL 일 경우 스왑 구현

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	pml4_set_page(&thread_current()->pml4, frame->page, frame->kva, page->writable);

	return swap_in (page, frame->kva);
}

/* Initialize new supplemental page table */
void /* process.c initd 에서 초기화 하고 있음*/
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
	hash_init(&spt->pages, page_hash, page_less, NULL);
}

/* Copy supplemental page table from src to dst */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {
	
	// TODO 구현
	// 존나 복잡할 듯
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
}
