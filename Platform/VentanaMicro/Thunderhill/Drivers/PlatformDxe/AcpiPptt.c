/** @file
  Create PPTT.

  Copyright (c) 2024, Ventana Micro Systems Inc. All Rights Reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "PlatformDxe.h"

STATIC UINT64  *mDeviceTree = NULL;

/** Operate a check on a Device Tree node.

  @param [in]  Fdt          Pointer to a Flattened Device Tree.
  @param [in]  NodeOffset   Offset of the node to compare input string.
  @param [in]  Context      Context to operate the check on the node.

  @retval True    The check is correct.
  @retval FALSE   Otherwise, or error.
**/
typedef
BOOLEAN
(EFIAPI *NODE_CHECKER_FUNC)(
  IN  CONST VOID    *Fdt,
  IN        INT32     NodeOffset,
  IN  CONST VOID    *Context
  );

#define PPTT_INIT_L1_DCACHE_STRUCTURE \
  { \
    EFI_ACPI_6_5_PPTT_TYPE_CACHE, \
    sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE), \
    { EFI_ACPI_RESERVED_BYTE, EFI_ACPI_RESERVED_BYTE }, \
    { \
      EFI_ACPI_6_5_PPTT_CACHE_SIZE_VALID, \
      EFI_ACPI_6_5_PPTT_NUMBER_OF_SETS_VALID, \
      EFI_ACPI_6_5_PPTT_ASSOCIATIVITY_VALID, \
      EFI_ACPI_6_5_PPTT_ALLOCATION_TYPE_VALID, \
      EFI_ACPI_6_5_PPTT_CACHE_TYPE_VALID, \
      EFI_ACPI_6_5_PPTT_WRITE_POLICY_VALID, \
      EFI_ACPI_6_5_PPTT_LINE_SIZE_VALID, \
      EFI_ACPI_6_5_PPTT_CACHE_ID_VALID, \
    }, \
    0, \
    SIZE_128KB, \
    256, \
    8, \
    { \
      EFI_ACPI_6_5_CACHE_ATTRIBUTES_ALLOCATION_READ_WRITE, \
      EFI_ACPI_6_5_CACHE_ATTRIBUTES_CACHE_TYPE_DATA, \
      EFI_ACPI_6_5_CACHE_ATTRIBUTES_WRITE_POLICY_WRITE_BACK, \
    }, \
    64, \
    1, \
  }

#define PPTT_INIT_L1_ICACHE_STRUCTURE \
  { \
    EFI_ACPI_6_5_PPTT_TYPE_CACHE, \
    sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE), \
    { EFI_ACPI_RESERVED_BYTE, EFI_ACPI_RESERVED_BYTE }, \
    { \
      EFI_ACPI_6_5_PPTT_CACHE_SIZE_VALID, \
      EFI_ACPI_6_5_PPTT_NUMBER_OF_SETS_VALID, \
      EFI_ACPI_6_5_PPTT_ASSOCIATIVITY_VALID, \
      EFI_ACPI_6_5_PPTT_ALLOCATION_TYPE_VALID, \
      EFI_ACPI_6_5_PPTT_CACHE_TYPE_VALID, \
      EFI_ACPI_6_5_PPTT_WRITE_POLICY_VALID, \
      EFI_ACPI_6_5_PPTT_LINE_SIZE_VALID, \
      EFI_ACPI_6_5_PPTT_CACHE_ID_VALID, \
    }, \
    0, \
    SIZE_512KB, \
    1024, \
    8, \
    { \
      EFI_ACPI_6_5_CACHE_ATTRIBUTES_ALLOCATION_READ, \
      EFI_ACPI_6_5_CACHE_ATTRIBUTES_CACHE_TYPE_INSTRUCTION, \
      0, \
    }, \
    64, \
    2, \
  }

#define PPTT_INIT_L2_CACHE_STRUCTURE \
  { \
    EFI_ACPI_6_5_PPTT_TYPE_CACHE, \
    sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE), \
    { EFI_ACPI_RESERVED_BYTE, EFI_ACPI_RESERVED_BYTE }, \
    { \
      EFI_ACPI_6_5_PPTT_CACHE_SIZE_VALID, \
      EFI_ACPI_6_5_PPTT_NUMBER_OF_SETS_VALID, \
      EFI_ACPI_6_5_PPTT_ASSOCIATIVITY_VALID, \
      EFI_ACPI_6_5_PPTT_ALLOCATION_TYPE_VALID, \
      EFI_ACPI_6_5_PPTT_CACHE_TYPE_VALID, \
      EFI_ACPI_6_5_PPTT_WRITE_POLICY_VALID, \
      EFI_ACPI_6_5_PPTT_LINE_SIZE_VALID, \
      EFI_ACPI_6_5_PPTT_CACHE_ID_VALID, \
    }, \
    0, \
    SIZE_1MB, \
    2048, \
    8, \
    { \
      EFI_ACPI_6_5_CACHE_ATTRIBUTES_ALLOCATION_READ_WRITE, \
      EFI_ACPI_6_5_CACHE_ATTRIBUTES_CACHE_TYPE_UNIFIED, \
      EFI_ACPI_6_5_CACHE_ATTRIBUTES_WRITE_POLICY_WRITE_BACK, \
    }, \
    64, \
    3, \
  }

#define PPTT_INIT_L3_CACHE_STRUCTURE \
  { \
    EFI_ACPI_6_5_PPTT_TYPE_CACHE, \
    sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE), \
    { EFI_ACPI_RESERVED_BYTE, EFI_ACPI_RESERVED_BYTE }, \
    { \
      EFI_ACPI_6_5_PPTT_CACHE_SIZE_VALID, \
      EFI_ACPI_6_5_PPTT_NUMBER_OF_SETS_VALID, \
      EFI_ACPI_6_5_PPTT_ASSOCIATIVITY_VALID, \
      EFI_ACPI_6_5_PPTT_ALLOCATION_TYPE_VALID, \
      EFI_ACPI_6_5_PPTT_CACHE_TYPE_VALID, \
      EFI_ACPI_6_5_PPTT_WRITE_POLICY_VALID, \
      EFI_ACPI_6_5_PPTT_LINE_SIZE_VALID, \
      EFI_ACPI_6_5_PPTT_CACHE_ID_VALID, \
    }, \
    0, \
    SIZE_128MB, \
    0x20000, \
    16, \
    { \
      EFI_ACPI_6_5_CACHE_ATTRIBUTES_ALLOCATION_READ_WRITE, \
      EFI_ACPI_6_5_CACHE_ATTRIBUTES_CACHE_TYPE_UNIFIED, \
      EFI_ACPI_6_5_CACHE_ATTRIBUTES_WRITE_POLICY_WRITE_BACK, \
    }, \
    64, \
    4, \
  }

#define PPTT_INIT_CPU_STRUCTURE \
  { \
    EFI_ACPI_6_5_PPTT_TYPE_PROCESSOR, \
    sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR) + 8, \
    { EFI_ACPI_RESERVED_BYTE, EFI_ACPI_RESERVED_BYTE }, \
    { \
      EFI_ACPI_6_5_PPTT_PACKAGE_NOT_PHYSICAL, \
      EFI_ACPI_6_5_PPTT_PROCESSOR_ID_VALID, \
      EFI_ACPI_6_5_PPTT_PROCESSOR_IS_NOT_THREAD, \
      EFI_ACPI_6_5_PPTT_NODE_IS_LEAF, \
      EFI_ACPI_6_5_PPTT_IMPLEMENTATION_IDENTICAL, \
    }, \
    0, \
    0, \
    2, \
  }

#define PPTT_INIT_CLUSTER_STRUCTURE \
  { \
    EFI_ACPI_6_5_PPTT_TYPE_PROCESSOR, \
    sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR), \
    { EFI_ACPI_RESERVED_BYTE, EFI_ACPI_RESERVED_BYTE }, \
    { \
      EFI_ACPI_6_5_PPTT_PACKAGE_NOT_PHYSICAL, \
      EFI_ACPI_6_5_PPTT_PROCESSOR_ID_INVALID, \
      EFI_ACPI_6_5_PPTT_PROCESSOR_IS_NOT_THREAD, \
      EFI_ACPI_6_5_PPTT_NODE_IS_NOT_LEAF, \
      EFI_ACPI_6_5_PPTT_IMPLEMENTATION_IDENTICAL, \
    }, \
    0, \
    0, \
    0, \
  }

EFI_ACPI_6_5_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_HEADER mPpttHeaderTemplate = 
{
  THUNDERHILL_ACPI_HEADER (
    EFI_ACPI_6_5_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_STRUCTURE_SIGNATURE,
    0, // fill in
   EFI_ACPI_6_5_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_REVISION 
  ),
};

/** Check whether a node has the input name.

  @param [in]  Fdt          Pointer to a Flattened Device Tree.
  @param [in]  Node         Offset of the node to check the name.
  @param [in]  SearchName   Node name to search.
                            This is a NULL terminated string.

  @retval True    The node has the input name.
  @retval FALSE   Otherwise, or error.
**/
STATIC
BOOLEAN
EFIAPI
FdtNodeHasName (
  IN  CONST VOID   *Fdt,
  IN        INT32  Node,
  IN  CONST VOID   *SearchName
  )
{
  CONST CHAR8  *NodeName;
  UINT32       Length;

  if ((Fdt == NULL) ||
      (SearchName == NULL))
  {
    ASSERT (0);
    return FALSE;
  }

  // Always compare the whole string. Don't stop at the "@" char.
  Length = (UINT32)AsciiStrLen (SearchName);

  // Get the address of the node name.
  NodeName = fdt_offset_ptr (Fdt, Node + FDT_TAGSIZE, Length + 1);
  if (NodeName == NULL) {
    return FALSE;
  }

  // SearchName must be longer than the node name.
  if (Length > AsciiStrLen (NodeName)) {
    return FALSE;
  }

  if (AsciiStrnCmp (NodeName, SearchName, Length) != 0) {
    return FALSE;
  }

  // The name matches perfectly, or
  // the node name is XXX@addr and the XXX matches.
  if ((NodeName[Length] == '\0') ||
      (NodeName[Length] == '@'))
  {
    return TRUE;
  }

  return FALSE;
}
/** Get the next node in the whole DT fulfilling a condition.

  The condition to fulfill is checked by the NodeChecker function.
  Context is passed to NodeChecker.

  The Device tree is traversed in a depth-first search, starting from Node.
  The input Node is skipped.

  @param [in]  Fdt              Pointer to a Flattened Device Tree.
  @param [in, out]  Node        At entry: Node offset to start the search.
                                          This first node is skipped.
                                          Write (-1) to search the whole tree.
                                At exit:  If success, contains the offset of
                                          the next node fulfilling the
                                          condition.
  @param [in, out]  Depth       Depth is incremented/decremented of the depth
                                difference between the input Node and the
                                output Node.
                                E.g.: If the output Node is a child node
                                of the input Node, contains (+1).
  @param [in]  NodeChecker      Function called to check if the condition
                                is fulfilled.
  @param [in]  Context          Context for the NodeChecker.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           No matching node found.
**/
STATIC
EFI_STATUS
EFIAPI
FdtGetNextCondNode (
  IN      CONST VOID               *Fdt,
  IN OUT        INT32              *Node,
  IN OUT        INT32              *Depth,
  IN            NODE_CHECKER_FUNC  NodeChecker,
  IN      CONST VOID               *Context
  )
{
  INT32  CurrNode;

  if ((Fdt == NULL)   ||
      (Node == NULL)  ||
      (Depth == NULL) ||
      (NodeChecker == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  CurrNode = *Node;
  do {
    CurrNode = fdt_next_node (Fdt, CurrNode, Depth);
    if ((CurrNode == -FDT_ERR_NOTFOUND) ||
        (*Depth < 0))
    {
      // End of the tree, no matching node found.
      return EFI_NOT_FOUND;
    } else if (CurrNode < 0) {
      // An error occurred.
      ASSERT (0);
      return EFI_ABORTED;
    }
  } while (!NodeChecker (Fdt, CurrNode, Context));

  // Matching node found.
  *Node = CurrNode;
  return EFI_SUCCESS;
}

/** Get the next node in a branch fulfilling a condition.

  The condition to fulfill is checked by the NodeChecker function.
  Context is passed to NodeChecker.

  The Device tree is traversed in a depth-first search, starting from Node.
  The input Node is skipped.

  @param [in]       Fdt             Pointer to a Flattened Device Tree.
  @param [in]       FdtBranch       Only search in the sub-nodes of this
                                    branch.
                                    Write (-1) to search the whole tree.
  @param [in]       NodeChecker     Function called to check if the condition
                                    is fulfilled.
  @param [in]       Context         Context for the NodeChecker.
  @param [in, out]  Node            At entry: Node offset to start the search.
                                         This first node is skipped.
                                         Write (-1) to search the whole tree.
                                    At exit:  If success, contains the offset
                                         of the next node in the branch
                                         fulfilling the condition.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           No matching node found.
**/
STATIC
EFI_STATUS
EFIAPI
FdtGetNextCondNodeInBranch (
  IN      CONST VOID               *Fdt,
  IN            INT32              FdtBranch,
  IN            NODE_CHECKER_FUNC  NodeChecker,
  IN      CONST VOID               *Context,
  IN OUT        INT32              *Node
  )
{
  EFI_STATUS  Status;
  INT32       CurrNode;
  INT32       Depth;

  if ((Fdt == NULL)   ||
      (Node == NULL)  ||
      (NodeChecker == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  CurrNode = FdtBranch;
  Depth    = 0;

  // First, check the Node is in the sub-nodes of the branch.
  // This allows to find the relative depth of Node in the branch.
  if (CurrNode != *Node) {
    for (CurrNode = fdt_next_node (Fdt, CurrNode, &Depth);
         (CurrNode >= 0) && (Depth > 0);
         CurrNode = fdt_next_node (Fdt, CurrNode, &Depth))
    {
      if (CurrNode == *Node) {
        // Node found.
        break;
      }
    } // for

    if ((CurrNode < 0) || (Depth <= 0)) {
      // Node is not a node in the branch, or an error occurred.
      ASSERT (0);
      return EFI_INVALID_PARAMETER;
    }
  }

  // Get the next node in the tree fulfilling the condition,
  // in any branch.
  Status = FdtGetNextCondNode (
             Fdt,
             Node,
             &Depth,
             NodeChecker,
             Context
             );
  if (EFI_ERROR (Status)) {
    ASSERT (Status == EFI_NOT_FOUND);
    return Status;
  }

  if (Depth <= 0) {
    // The node found is not in the right branch.
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/** Get the next node in a branch having a matching name.

  The Device tree is traversed in a depth-first search, starting from Node.
  The input Node is skipped.

  @param [in]       Fdt         Pointer to a Flattened Device Tree.
  @param [in]       FdtBranch   Only search in the sub-nodes of this branch.
                                Write (-1) to search the whole tree.
  @param [in]       NodeName    The node name to search.
                                This is a NULL terminated string.
  @param [in, out]  Node        At entry: Node offset to start the search.
                                          This first node is skipped.
                                          Write (-1) to search the whole tree.
                                At exit:  If success, contains the offset of
                                          the next node in the branch
                                          having a matching name.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           No matching node found.
**/
STATIC
EFI_STATUS
EFIAPI
FdtGetNextNamedNodeInBranch (
  IN      CONST VOID   *Fdt,
  IN            INT32  FdtBranch,
  IN      CONST CHAR8  *NodeName,
  IN OUT        INT32  *Node
  )
{
  return FdtGetNextCondNodeInBranch (
           Fdt,
           FdtBranch,
           FdtNodeHasName,
           NodeName,
           Node
           );
}

/** Count the number of Device Tree nodes fulfilling a condition
    in a Device Tree branch.

  The condition to fulfill is checked by the NodeChecker function.
  Context is passed to NodeChecker.

  @param [in]  Fdt              Pointer to a Flattened Device Tree.
  @param [in]  FdtBranch        Only search in the sub-nodes of this branch.
                                Write (-1) to search the whole tree.
  @param [in]  NodeChecker      Function called to check the condition is
                                fulfilled.
  @param [in]  Context          Context for the NodeChecker.
  @param [out] NodeCount        If success, contains the count of nodes
                                fulfilling the condition.
                                Can be 0.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
FdtCountCondNodeInBranch (
  IN  CONST VOID               *Fdt,
  IN        INT32              FdtBranch,
  IN        NODE_CHECKER_FUNC  NodeChecker,
  IN  CONST VOID               *Context,
  OUT       UINT32             *NodeCount
  )
{
  EFI_STATUS  Status;
  INT32       CurrNode;

  if ((Fdt == NULL)         ||
      (NodeChecker == NULL) ||
      (NodeCount == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  *NodeCount = 0;
  CurrNode   = FdtBranch;
  while (TRUE) {
    Status = FdtGetNextCondNodeInBranch (
               Fdt,
               FdtBranch,
               NodeChecker,
               Context,
               &CurrNode
               );
    if (EFI_ERROR (Status)  &&
        (Status != EFI_NOT_FOUND))
    {
      ASSERT (0);
      return Status;
    } else if (Status == EFI_NOT_FOUND) {
      break;
    }

    (*NodeCount)++;
  }

  return EFI_SUCCESS;
}

/** Count the number of nodes in a branch with the input name.

  @param [in]  Fdt              Pointer to a Flattened Device Tree.
  @param [in]  FdtBranch        Only search in the sub-nodes of this branch.
                                Write (-1) to search the whole tree.
  @param [in]  NodeName         Node name to search.
                                This is a NULL terminated string.
  @param [out] NodeCount        If success, contains the count of nodes
                                fulfilling the condition.
                                Can be 0.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
FdtCountNamedNodeInBranch (
  IN  CONST VOID    *Fdt,
  IN        INT32   FdtBranch,
  IN  CONST CHAR8   *NodeName,
  OUT       UINT32  *NodeCount
  )
{
  return FdtCountCondNodeInBranch (
           Fdt,
           FdtBranch,
           FdtNodeHasName,
           NodeName,
           NodeCount
           );
}

STATIC
UINTN
PpttGenerate (VOID *NodePtr, UINT32 Offset)
{
  UINT32      CpuNodeCount, Index;
  EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE L3Cache = PPTT_INIT_L3_CACHE_STRUCTURE;
  EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE L2Cache = PPTT_INIT_L2_CACHE_STRUCTURE;
  EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE L1ICache = PPTT_INIT_L1_ICACHE_STRUCTURE;
  EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE L1DCache = PPTT_INIT_L1_DCACHE_STRUCTURE;
  EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR Core = PPTT_INIT_CPU_STRUCTURE;
  EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR Cluster = PPTT_INIT_CLUSTER_STRUCTURE;
  INT32 CpusNode, CpuNode, L2CacheNode, L3CacheNode;
  UINTN SizeGenerated;
  UINT32 L3CacheOffset, L2CacheOffset, L1DCacheOffset, L1ICacheOffset, Cluster0Offset;
  VOID  *FdtPointer, *L3CachePtr, *L2CachePtr, *L1DCachePtr, *L1ICachePtr, *Cluster0Ptr, *New;
  UINT32 *PrivateResourcePtr;
  EFI_STATUS                                          Status;
  INT32 Len, Phandle;
  CONST VOID *Prop;

  SizeGenerated = 0;
  FdtPointer = mDeviceTree;

  // The "cpus" node resides at the root of the DT. Fetch it.
  CpusNode = fdt_path_offset (FdtPointer, "/cpus");
  if (CpusNode < 0) {
    return EFI_NOT_FOUND;
  }

  L3CachePtr = (EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE *)NodePtr;
  L3CacheOffset = Offset; // 1st entry is L3 cache
  SizeGenerated += sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE);

  Cluster0Offset = L3CacheOffset + sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE);
  Cluster0Ptr = (EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR *) (L3CachePtr + sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE));
  SizeGenerated += sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR);

  // Populate Cluster Node
  CopyMem ((VOID *)Cluster0Ptr, (VOID *)&Cluster, sizeof (Cluster));
  New = Cluster0Ptr + sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR);

  // Count the number of "cpu" nodes under the "cpus" node.
  Status = FdtCountNamedNodeInBranch (FdtPointer, CpusNode, "cpu", &CpuNodeCount);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  if (CpuNodeCount == 0) {
    ASSERT (0);
    return EFI_NOT_FOUND;
  }

  CpuNode = CpusNode;
  for (Index = 0; Index < CpuNodeCount; Index++) {
    Status = FdtGetNextNamedNodeInBranch (FdtPointer, CpusNode, "cpu", &CpuNode);
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      if (Status == EFI_NOT_FOUND) {
        // Should have found the node.
        Status = EFI_ABORTED;
      }
      return Status;
    }

    L2CacheOffset = New - (NodePtr - sizeof (mPpttHeaderTemplate));
    L2CachePtr = (EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE *) New;
    SizeGenerated += sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE);

    L1ICacheOffset = L2CacheOffset + sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE);
    L1ICachePtr = (EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE *) (L2CachePtr + sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE));
    SizeGenerated += sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE);

    L1DCacheOffset = L1ICacheOffset + sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE);
    L1DCachePtr = (EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE *) (L1ICachePtr + sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE));
    SizeGenerated += sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE);
    New = L1DCachePtr + sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE);

    Core.AcpiProcessorId = Index;
    CopyMem ((VOID *)New, (VOID *)&Core, sizeof (Core));
    New += sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR);
    PrivateResourcePtr    = (UINT32 *)New;
    PrivateResourcePtr[0] = L1ICacheOffset;
    PrivateResourcePtr[1] = L1DCacheOffset;
    New                   += 2 * sizeof (UINT32);
    SizeGenerated += sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR) + 2 * sizeof (UINT32);

    Prop = fdt_getprop (FdtPointer, CpuNode, "i-cache-size", &Len);
    if (Prop) {
      L1ICache.Size = fdt32_to_cpu (*(UINT32 *)Prop);
      DEBUG((DEBUG_INFO, "%a: Found i-cache-size (0x%x) in DT for CPU%d\n", __func__, L1ICache.Size, Index));
      Prop = fdt_getprop (FdtPointer, CpuNode, "i-cache-line-size", &Len);
      if (Prop) {
        L1ICache.LineSize = fdt32_to_cpu (*(UINT32 *)Prop);
        DEBUG((DEBUG_INFO, "%a: Found i-cache-line-size(0x%x) in DT for CPU%d\n", __func__, L1ICache.LineSize, Index));
      }
      Prop = fdt_getprop (FdtPointer, CpuNode, "i-cache-sets", &Len);
      if (Prop) {
        L1ICache.NumberOfSets = fdt32_to_cpu (*(UINT32 *)Prop);
        DEBUG((DEBUG_INFO, "%a: Found i-cache-sets (%d) in DT for CPU%d\n", __func__, L1ICache.NumberOfSets, Index));
      }
      L1ICache.Associativity = (L1ICache.Size / L1ICache.NumberOfSets) / L1ICache.LineSize;
    }
    L1ICache.NextLevelOfCache = L2CacheOffset;
    CopyMem ((VOID *)L1ICachePtr, (VOID *)&L1ICache, sizeof (L1ICache));

    Prop = fdt_getprop (FdtPointer, CpuNode, "d-cache-size", &Len);
    if (Prop) {
      L1DCache.Size = fdt32_to_cpu (*(UINT32 *)Prop);
      DEBUG((DEBUG_INFO, "%a: Found d-cache-size (0x%x) in DT for CPU%d\n", __func__, L1DCache.Size, Index));
      Prop = fdt_getprop (FdtPointer, CpuNode, "d-cache-line-size", &Len);
      if (Prop) {
        L1DCache.LineSize = fdt32_to_cpu (*(UINT32 *)Prop);
        DEBUG((DEBUG_INFO, "%a: Found d-cache-Line-size (0x%x) in DT for CPU%d\n", __func__, L1DCache.LineSize, Index));
      }
      Prop = fdt_getprop (FdtPointer, CpuNode, "d-cache-sets", &Len);
      if (Prop) {
        L1DCache.NumberOfSets = fdt32_to_cpu (*(UINT32 *)Prop);
        DEBUG((DEBUG_INFO, "%a: Found d-cache-sets (%d) in DT for CPU%d\n", __func__, L1DCache.NumberOfSets, Index));
      }
      L1DCache.Associativity = (L1DCache.Size / L1DCache.NumberOfSets) / L1DCache.LineSize;
    }
    L1DCache.NextLevelOfCache = L2CacheOffset;
    CopyMem ((VOID *)L1DCachePtr, (VOID *)&L1DCache, sizeof (L1DCache));

    Prop = fdt_getprop (FdtPointer, CpuNode, "next-level-cache", &Len);
    if (Prop) {
      Phandle = fdt32_to_cpu (ReadUnaligned32 ((const UINT32 *)Prop));
      L2CacheNode = fdt_node_offset_by_phandle (FdtPointer, Phandle);
      Prop = fdt_getprop (FdtPointer, L2CacheNode, "d-cache-size", &Len);
      if (Prop) {
        L2Cache.Size = fdt32_to_cpu (*(UINT32 *)Prop);
        DEBUG((DEBUG_INFO, "%a: Found d-cache-size (0x%x) in DT for L2 of CPU%d\n", __func__, L2Cache.Size, Index));
        Prop = fdt_getprop (FdtPointer, L2CacheNode, "d-cache-line-size", &Len);
        if (Prop) {
          L2Cache.LineSize = fdt32_to_cpu (*(UINT32 *)Prop);
          DEBUG((DEBUG_INFO, "%a: Found d-cache-Line-size (0x%x) in DT for L2 of CPU%d\n", __func__, L2Cache.LineSize, Index));
        }
        Prop = fdt_getprop (FdtPointer, L2CacheNode, "d-cache-sets", &Len);
        if (Prop) {
          L2Cache.NumberOfSets = fdt32_to_cpu (*(UINT32 *)Prop);
          DEBUG((DEBUG_INFO, "%a: Found d-cache-sets (%d) in DT for L2 of CPU%d\n", __func__, L2Cache.NumberOfSets, Index));
        }
        L2Cache.Associativity = (L2Cache.Size / L2Cache.NumberOfSets) / L2Cache.LineSize;
      } else {
        Prop = fdt_getprop (FdtPointer, L2CacheNode, "cache-size", &Len);
        if (Prop) {
          L2Cache.Size = fdt32_to_cpu (*(UINT32 *)Prop);
          DEBUG((DEBUG_INFO, "%a: Found cache-size (0x%x) in DT for L2 of CPU%d\n", __func__, L2Cache.Size, Index));
          Prop = fdt_getprop (FdtPointer, L2CacheNode, "cache-line-size", &Len);
          if (Prop) {
            L2Cache.LineSize = fdt32_to_cpu (*(UINT32 *)Prop);
            DEBUG((DEBUG_INFO, "%a: Found cache-Line-size (0x%x) in DT for L2 of CPU%d\n", __func__, L2Cache.LineSize, Index));
          }
          Prop = fdt_getprop (FdtPointer, L2CacheNode, "cache-sets", &Len);
          if (Prop) {
            L2Cache.NumberOfSets = fdt32_to_cpu (*(UINT32 *)Prop);
            DEBUG((DEBUG_INFO, "%a: Found cache-sets (%d) in DT for L2 of CPU%d\n", __func__, L2Cache.NumberOfSets, Index));
          }
          L2Cache.Associativity = (L2Cache.Size / L2Cache.NumberOfSets) / L2Cache.LineSize;
        }
      }
    } else {
      DEBUG((DEBUG_ERROR, "%a: L2 cache not found in DT?\n", __func__));
    }
    L2Cache.NextLevelOfCache = L3CacheOffset;
    CopyMem ((VOID *)L2CachePtr, (VOID *)&L2Cache, sizeof (L2Cache));

    Prop = fdt_getprop (FdtPointer, L2CacheNode, "next-level-cache", &Len);
    if (Prop) {
      Phandle = fdt32_to_cpu (ReadUnaligned32 ((const UINT32 *)Prop));
      L3CacheNode = fdt_node_offset_by_phandle (FdtPointer, Phandle);
      Prop = fdt_getprop (FdtPointer, L3CacheNode, "cache-size", &Len);
      if (Prop) {
        L3Cache.Size = fdt32_to_cpu (*(UINT32 *)Prop);
        DEBUG((DEBUG_INFO, "%a: Found cache-size (0x%x) in DT for L3 of CPU%d\n", __func__, L3Cache.Size, Index));
        Prop = fdt_getprop (FdtPointer, L3CacheNode, "cache-line-size", &Len);
        if (Prop) {
          L3Cache.LineSize = fdt32_to_cpu (*(UINT32 *)Prop);
          DEBUG((DEBUG_INFO, "%a: Found cache-Line-size (0x%x) in DT for L3 of CPU%d\n", __func__, L3Cache.LineSize, Index));
        }
        Prop = fdt_getprop (FdtPointer, L3CacheNode, "cache-sets", &Len);
        if (Prop) {
          L3Cache.NumberOfSets = fdt32_to_cpu (*(UINT32 *)Prop);
          DEBUG((DEBUG_INFO, "%a: Found cache-sets (%d) in DT for L3 of CPU%d\n", __func__, L3Cache.NumberOfSets, Index));
        }
        L3Cache.Associativity = (L3Cache.Size / L3Cache.NumberOfSets) / L3Cache.LineSize;
      } else {
        DEBUG((DEBUG_ERROR, "%a: L3 cache property not found in DT?\n", __func__));
      }
    } else {
      DEBUG((DEBUG_ERROR, "%a: L3 cache-size not found in DT?\n", __func__));
    }
    L3Cache.NextLevelOfCache = 0;
    CopyMem ((VOID *)L3CachePtr, (VOID *)&L3Cache, sizeof (L3Cache));
  }

  return SizeGenerated;
}

/*
 *  Install PPTT table.
 */
EFI_STATUS
AcpiInstallPpttTable (
  VOID
  )
{
  EFI_ACPI_6_5_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_HEADER *PpttPtr;
  EFI_ACPI_TABLE_PROTOCOL                             *AcpiTableProtocol;
  UINTN                                               PpttTableKey  = 0;
  EFI_STATUS                                          Status;
  UINTN                                               MaxLength, Length;
  VOID                                                *Hob, *NodePtr;

  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID **)&AcpiTableProtocol
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Hob = GetFirstGuidHob (&gFdtHobGuid);
  if ((Hob == NULL) || (GET_GUID_HOB_DATA_SIZE (Hob) != sizeof (UINT64))) {
    return EFI_NOT_FOUND;
  }

  mDeviceTree = (VOID *)(UINTN)*(UINT64 *)GET_GUID_HOB_DATA (Hob);

  if (fdt_check_header (mDeviceTree) != 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: No DTB found @ 0x%p\n",
      __func__,
      mDeviceTree
      ));
    return EFI_NOT_FOUND;
  }

  // Calculate maximum size
  MaxLength = sizeof (EFI_ACPI_DESCRIPTION_HEADER) +
         sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE) +
         sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR)  +
         THUNDERHILL_NUM_CORE * ((sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE) * 3) +
                sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR) + sizeof (UINT32) * 2);

  PpttPtr = (EFI_ACPI_6_5_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_HEADER *)AllocateZeroPool (MaxLength);
  if (PpttPtr == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  CopyMem (
    PpttPtr,
    &mPpttHeaderTemplate,
    sizeof (mPpttHeaderTemplate)
    );  
  Length = sizeof (mPpttHeaderTemplate);

  // Generate Cache nodes (identical cores share the same cache nodes)
  NodePtr = (VOID *)PpttPtr + Length;
  Length += PpttGenerate (NodePtr, Length);

  // Update checksum
  ASSERT (MaxLength >= Length);
  PpttPtr->Header.Length = Length;
  AcpiUpdateChecksum ((UINT8 *)PpttPtr, PpttPtr->Header.Length);

  Status = AcpiTableProtocol->InstallAcpiTable (
                                AcpiTableProtocol,
                                (VOID *)PpttPtr,
                                PpttPtr->Header.Length,
                                &PpttTableKey
                                );
  FreePool ((VOID *)PpttPtr);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}
