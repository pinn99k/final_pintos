/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"

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

	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page (spt, upage) == NULL) {
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */

		/* TODO: Insert the page into the spt. */
		spt_insert_page(spt, upage);
	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
	struct page *page = NULL;
	
	/* TODO: Fill this function. */
	struct page *page = (struct page*)malloc(sizeof(struct page));
	struct hash_elem *e;

	// va의 hash_elem
	page->va = va;
	
	/* 못찾으면 NULL 리턴 */
	e = hash_find(&spt->pages, &page->hash_elem);
	free(page);
	
	// 있으면 e에 해당하는 페이지 반환
	return e!=NULL ? hash_entry(e, struct page, hash_elem) : NULL;
}

/* Insert PAGE into spt with validation. */
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
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
	
	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);
	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
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
	/*
	> 메모리 참조가 유효한지 확인
	> 가상 메모리 페이지에 들어갈 콘텐츠 위치 찾기
	> 파일 or 스왑 또는 모든 페이지가 0일 수 있음
	> 공유 페이지의 경우 페이지 프레임엔 있지만 페이지 테이블엔 없을 수 있음
	> 잘못된 접근 프로세스를 종료하는 경우
	> 1. 유효하지 않은 사용자 주소
	> 2. 커널 주소
	> 3. 권한오류(읽기 전용 페이지에 쓰기 시도)
	> 페이지 프레임을 할당
	> 디스크에스 페이지 프레임으로 데이터 가져오고
	> 페이지 테이블을 업데이트
	*/
	// 페이지 찾아오기
	struct page *page = spt_find_page(spt, addr);
	if(page == NULL){
		// 둘 중 뭔지 모르겠음
		thread_exit();
		return false;
	}
	if(!not_present){ // 데이터를 기대하지 말아야 한다고 표시
		// 둘 중 뭔지 모르겠음
		thread_exit();
		return false;
	}
	if(addr == NULL || !is_user_vaddr(addr) || !pml4_get_page(thread_current()->pml4, addr)){
		/*
		페이지가 커널 가상 메모리 내에 있는 경우
		액세스가 읽기 전용 페이지에 쓰기를 시도하는 경우
		*/
		// 둘 중 뭔지 모르겠음
		thread_exit();
		return false;
	}
	if(!write){
		// 둘 중 뭔지 모르겠음
		thread_exit();
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

	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	 
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
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
}
