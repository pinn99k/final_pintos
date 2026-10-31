/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "threads/vaddr.h"

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
unsigned page_hash (const struct hash_elem *p_, void *aux UNUSED);
bool page_less (const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED);

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
		page->writable = writable;
		page->isFlag = type & VM_MARKER_0;

		/* TODO: Insert the page into the spt. */
		if (spt_insert_page(spt, page) == false){
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
	struct page *page = NULL; // NULL로 초기화
	struct hash_iterator i;
	hash_first (&i, spt);
	
	// 페이지와 연결
	while (hash_next (&i)){
		// 페이지 엔트리에 넣기
		page = hash_entry (hash_cur(&i), struct page, hash_elem);

		// 맞으면 리턴 page
		if ((uint64_t)page->va == pg_round_down(va)){
			return page;
		}
	}

	// 다틀렸으면 NULL
	return NULL;
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
	if (hash_insert(&spt->pages, &page->hash_elem) == NULL){
		return true;
	};

	return false;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	hash_delete(&spt->pages, &page->hash_elem); // 고민
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
	struct frame *frame = malloc(sizeof(struct frame)); 
	/* TODO: Fill this function. */

	/* 
	말록 해서 프레임 공간 가져오면 되는거 아님? 
	KVA를 가져와서 넣어줘야 하는거 아닌가?
	*/

	// 어디서 초기화 하지?
	// 프레임 지울때? 스왑할때 빼야할 듯

	// 이걸 이렇게 가져오는게 맞나?
	frame->kva = palloc_get_page(PAL_USER);
	frame->page = NULL;
	if(frame->kva == NULL){
		//페이지 스왑
		return NULL;
	}
	
	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);
	
	// 아니 프레임 리스트로 연결 하긴 해야하는데 이걸 어디서 해야할지 모르겠어요;;
	// list_push_back(&frame_list, &frame->frame_elem);
	return frame;
}

/* Growing the stack. */
// bool 타입으로 변경
static bool
vm_stack_growth (void *addr UNUSED) {
	// addr을 기준으로 PGSIZE만큼 확장하면 되나?
	void *stack_page = pg_round_down(addr);

	if(vm_alloc_page(VM_ANON, stack_page, true)){
		return vm_claim_page(stack_page);
	}
	return false;
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
	if (addr == NULL || !is_user_vaddr(addr)){
		return false;
	}

	if (!not_present){
		return false;
	}
	
	struct page *page = spt_find_page(spt, addr);
	if(page == NULL){
	
		void *rsp = f->rsp;
		// 유저와 커널에 맞게 rsp를 다르게 사용
		if(!user){
			rsp = thread_current()->user_rsp;
		}

		// userstack과 최대크기 사이에 존재하면서 rsp 보다 높은 주소에 있을때
		// Pintos에서는 최대 스택 크기를 1MB(1,048,576 bytes)로 제한
		// USER_STACK - 1MB ≤ addr < USER_STACK
		// rsp-8 <= addr
		if (((USER_STACK - (1<<20))) <= addr && rsp - 8 <= addr && (addr < USER_STACK)){
			return vm_stack_growth(addr);
		}
		
		return false;
	}

	if (write && !page->writable){
		return false;
	}

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
	// if(page ==NULL){
	// 	return false;
	// }

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
	if(!pml4_set_page(thread_current()->pml4, page->va, frame->kva, page->writable)){
		page->frame = NULL;
		palloc_free_page(frame->kva);
		free(frame);
		return false;
	}

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
	
	struct hash_iterator i;
	struct page *parent_page; 	/* 부모 페이지 */
	struct page *child_page; 	/* 자식 페이지*/
	struct vm_entry *child_aux = NULL;  

	hash_first(&i, src);

	/* 부모 SPT 순회 */
	while (hash_next (&i)) {
		parent_page = hash_entry (hash_cur(&i), struct page, hash_elem);

		/* 부모 페이지 타입 판별 */
		switch (VM_TYPE (parent_page->operations->type))
		{
		case VM_UNINIT :
			/* 자식 aux 만들기 */
			child_aux = malloc (sizeof(struct vm_entry));
			memcpy (child_aux, parent_page->uninit.aux, sizeof(struct vm_entry));
			child_aux->file = thread_current ()->running_file;

			/* 자식 spt 등록 */
			if (!vm_alloc_page_with_initializer (parent_page->uninit.type, 
				parent_page->va, parent_page->writable, 
				parent_page->uninit.page_initializer, parent_page->uninit.aux)) {
				free (child_aux);
				return false;
			}					

			break;
		case VM_ANON :
			/* */
			

			break;
		case VM_FILE :
			break;

		default:
			return false;
		}
	}


	return true;
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
}

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
