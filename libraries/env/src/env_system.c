#include <nitro.h>

SDK_WEAK_SYMBOL ENVResource *resourceArray[] = { NULL };
static BOOL sInitialized = FALSE;
static char sCurrentClass[ENV_CLASS_NAME_MAX + 1];
static ENVResourceSetLink *sResourceSetLinkHead = NULL;
static ENVResourceSetLink sResourceSetLink[ENV_RESOURCE_SET_MAX];

static void ENVi_InitResourceSet(ENVResource *resSet);
static ENVResource *ENVi_SearchByMemberNameFromResourceSet(ENVResource *resSet, const char *memberName);
static ENVResource *ENVi_SearchByFullNameFromResourceSet(ENVResource *resSet, const char *fullName);
static void ENVi_SetNextSetForIter(ENVResourceIter *iter);

static void ENVi_ClearLinkList (void)
{
	MI_CpuClear8(&sResourceSetLink[0], sizeof(sResourceSetLink));
}

static ENVResourceSetLink *ENVi_SearchBlankLink (void)
{
	int n;

	for (n = 0; n < ENV_RESOURCE_SET_MAX; n++) {
		if (!sResourceSetLink[n].array) {
			return &sResourceSetLink[n];
		}
	}

	return NULL;
}

void ENV_Init (void)
{
	if (!sInitialized) {
		sInitialized = TRUE;

		ENVi_ClearLinkList();
		ENV_SetResoruceSetArray(resourceArray);
	}
}

void ENV_AppendResourceSet (ENVResource *resSet)
{
	ENVResourceSetLink *p = sResourceSetLinkHead;
	ENVResourceSetLink *lastp = NULL;
	ENVResourceSetLink *blank = ENVi_SearchBlankLink();

	if (!blank) {
		return;
	}

	while (p) {
		lastp = p;
		p = p->next;
	}

	blank->array = resSet;
	blank->next = NULL;

	if (lastp) {
		lastp->next = blank;
	} else {
		sResourceSetLinkHead = blank;
	}
}

void ENV_PrependResourceSet (ENVResource *resSet)
{
	ENV_InsertResourceSet(NULL, resSet);
}

void ENV_InsertResourceSet (ENVResource *standardResSet, ENVResource *resSet)
{
	ENVResourceSetLink *blank = ENVi_SearchBlankLink();

	if (!blank) {
		return;
	}

	blank->array = resSet;

	if (!standardResSet) {
		blank->next = sResourceSetLinkHead;
		sResourceSetLinkHead = blank;
	} else {
		ENVResourceSetLink *p = sResourceSetLinkHead;
		ENVResourceSetLink *lastp = NULL;

		while (p) {
			if (p->array == standardResSet) {
				break;
			}

			p = p->next;
		}

		if (!p) {
			return;
		}

		blank->next = p->next;
		p->next = blank;
	}
}

void ENV_SetResourceSet (ENVResource *resSet)
{
	ENVi_ClearLinkList();
	ENV_AppendResourceSet(resSet);
}

ENVResourceSetLink *ENV_GetResourceSetLinkHead (void)
{
	return sResourceSetLinkHead;
}

ENVResourceSetLink *ENV_GetNextResourceSet (ENVResourceSetLink *link)
{
	SDK_ASSERT(link);
	return link->next;
}

void ENV_SetClass (const char *className)
{
	SDK_WARNING(STD_GetStringLength(className) <= ENV_CLASS_NAME_MAX, "class name is too long");

	(void)STD_CopyLString(sCurrentClass, className, ENV_CLASS_NAME_MAX);
	sCurrentClass[ENV_CLASS_NAME_MAX] = '\0';
}

char *ENV_GetClass (void)
{
	return sCurrentClass;
}

static char *ENVi_CheckIfSameClass (char *name, char *className)
{
	char *s1 = className;
	char *s2 = name;

	while (*s1) {
		if (*s1++ != *s2++) {
			return NULL;
		}
	}

	if (*s2 != '.') {
		return NULL;
	}

	return s2 + 1;
}

static char *ENVi_CheckIfSameMember (char *name, char *memberName)
{
	char *s1 = memberName;
	char *s2 = name;

	while (*s2 && *s2 != '.') {
		s2++;
	}

	if (!*s2) {
		return NULL;
	}

	s2++;

	while (*s1) {
		if (*s1++ != *s2++) {
			return NULL;
		}
	}

	if (*s2 != '.' && *s2 != '\0') {
		return NULL;
	}

	return s2 + 1;
}

static ENVResource *ENVi_SearchByMemberNameFromResourceSet (ENVResource *resSet, const char *memberName)
{
	ENVResource *ir = resSet;

	for (; ir->name; ir++) {
		char *memberPtr = ENVi_CheckIfSameClass(ir->name, sCurrentClass);

		if (memberPtr && !STD_CompareString(memberPtr, memberName)) {
			return ir;
		}
	}

	return NULL;
}

ENVResource *ENVi_SearchByMemberName (const char *memberName, ENVResource **resSetPtr)
{
	ENVResourceSetLink *link = sResourceSetLinkHead;

	if (link && memberName && *memberName != '\0') {
		while (link) {
			ENVResource *p = ENVi_SearchByMemberNameFromResourceSet(link->array, memberName);
			if (p) {
				if (resSetPtr) {
					*resSetPtr = link->array;
				}
				return p;
			}
			link = link->next;
		}
	}

	return NULL;
}

static ENVResource *ENVi_SearchByFullNameFromResourceSet (ENVResource *resSet, const char *fullName)
{
	ENVResource *ir = resSet;

	for (; ir->name; ir++) {
		if (!STD_CompareString(ir->name, fullName)) {
			return ir;
		}
	}

	return NULL;
}

ENVResource *ENVi_SearchByFullName (const char *fullName, ENVResource **resSetPtr)
{
	ENVResourceSetLink *link = sResourceSetLinkHead;

	if (link && fullName && *fullName != '\0') {
		while (link) {
			ENVResource *p = ENVi_SearchByFullNameFromResourceSet(link->array, fullName);
			if (p) {
				if (resSetPtr) {
					*resSetPtr = link->array;
				}
				return p;
			}
			link = link->next;
		}
	}

	return NULL;
}

ENVResource *ENVi_Search (const char *name, ENVResource **resSetPtr)
{
	ENVResourceSetLink *link = sResourceSetLinkHead;

	if (link && name && *name != '\0') {
		if (*name == '.') {
			return ENVi_SearchByMemberName(name + 1, resSetPtr);
		} else {
			return ENVi_SearchByFullName(name, resSetPtr);
		}
	}

	return NULL;
}

void *ENVi_GetPtrAndLength (const char *name, int *len)
{
	ENVResource *resSetPtr;
	ENVResource *p = ENVi_Search(name, &resSetPtr);

	if (!p) {
		if (len) {
			*len = 0;
		}
		return NULL;
	}

	if (len) {
		*len = p->len - ((p->type == ENV_RESTYPE_STRING) ? 1 : 0);
	}

	if (p->type & ENV_RESTYPE_OFFSET_MASK) {
		return (void *)((u32)resSetPtr + (u32)(p->ptr));
	} else {
		return p->ptr;
	}
}

ENVType ENV_GetType (const char *name)
{
	ENVResource *p = ENV_Search(name);
	return (p) ? p->type : ENV_RESTYPE_NONE;
}

int ENV_GetSize (const char *name)
{
	ENVResource *p = ENV_Search(name);
	return (p) ? p->len : 0;
}

ENVResource *ENV_GetBelongingResourceSet (ENVResource *res)
{
	ENVResourceSetLink *link = sResourceSetLinkHead;

	while (link) {
		ENVResource *resSet = link->array;
		ENVResource *p = resSet;

		while (p->name) {
			p++;
		}

		if ((u32)resSet <= (u32)res && (u32)res <= (u32)p) {
			return resSet;
		}

		link = link->next;
	}

	return NULL;
}

BOOL ENV_GetU64 (const char *name, u64 *retVal)
{
	ENVResource *resSetPtr;
	ENVResource *p = ENVi_Search(name, &resSetPtr);

	if (!p) {
		*retVal = 0;
		return FALSE;
	}

	if (p->type & ENV_RESTYPE_OFFSET_MASK) {
		*retVal = *(u64 *)((u32)resSetPtr + (u32)(p->ptr));
	} else {
		SDK_WARNING((p + 1)->name && !STD_CompareString((p + 1)->name, ENVi_DUMMY_FOR_U64), "64bit resource is stored illegally");
		*retVal = (u64)(((u64)((p + 1)->ptr) << 32) | (u32)(p->ptr));
	}

	return TRUE;
}

void ENV_InitIter (ENVResourceIter *iter)
{
	iter->link = sResourceSetLinkHead;
	iter->ptr = iter->link->array;
	iter->count = 0;
}

static void ENVi_SetNextSetForIter (ENVResourceIter *iter)
{
	iter->link = iter->link->next;

	if (!iter->link) {
		return;
	}

	iter->ptr = iter->link->array;
}

ENVResource *ENV_SearchByClass (ENVResourceIter *iter, const char *className)
{
	while (iter->link) {
		if (!iter->ptr->name) {
			ENVi_SetNextSetForIter(iter);
			if (!iter->link) {
				break;
			}
		}

		if (iter->ptr->name) {
			char *memberPtr;

			ENVResource *p = iter->ptr;
			iter->ptr++;

			if ((memberPtr = ENVi_CheckIfSameClass(p->name, (char *)className)) != 0) {
				iter->supData = (void *)memberPtr;
				iter->count++;
				return p;
			}
		}
	}

	return NULL;
}

ENVResource *ENV_SearchByMember (ENVResourceIter *iter, const char *memberName)
{
	while (iter->link) {
		if (!iter->ptr->name) {
			ENVi_SetNextSetForIter(iter);
			if (!iter->link) {
				break;
			}
		}

		if (iter->ptr->name) {
			ENVResource *p = iter->ptr;
			iter->ptr++;

			if (ENVi_CheckIfSameMember(p->name, (char *)memberName)) {
				iter->supData = (void *)p;
				iter->count++;
				return p;
			}
		}
	}

	return NULL;
}

ENVResource *ENV_SearchByType (ENVResourceIter *iter, ENVType type)
{
	while (iter->link) {
		if (!iter->ptr->name) {
			ENVi_SetNextSetForIter(iter);
			if (!iter->link) {
				break;
			}
		}

		if (iter->ptr->name) {
			ENVResource *p = iter->ptr;
			iter->ptr++;

			if (p->type == type) {
				iter->count++;
				return p;
			}
		}
	}

	return NULL;
}

ENVResource *ENV_SearchByPartialName (ENVResourceIter *iter, const char *partialName)
{
	while (iter->link) {
		if (!iter->ptr->name) {
			ENVi_SetNextSetForIter(iter);
			if (!iter->link) {
				break;
			}
		}

		if (iter->ptr->name) {
			ENVResource *p = iter->ptr;
			iter->ptr++;

			if (STD_SearchString(p->name, partialName) != NULL) {
				iter->count++;
				return p;
			}
		}
	}

	return NULL;
}

void ENV_SetResoruceSetArray (ENVResource *array[])
{
	int n = 0;
	ENVResource *p = array[n];

	while (p != NULL) {
		if (n == 0) {
			ENV_SetResourceSet(p);
		} else {
			ENV_AppendResourceSet(p);
		}
		p = array[++n];
	}
}