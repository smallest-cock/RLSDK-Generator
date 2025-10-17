#include "pch.hpp"
#include "PiecesOfCode.hpp"

/*
# ========================================================================================= #
# Pieces Of Code
# ========================================================================================= #
*/

namespace ExtraPiecesOfCode
{
auto GMallocCompileMsg = R"---(#ifdef USE_GMALLOC
#pragma message("Compiling with GMalloc support...")
#else
#pragma message("Compiling without GMalloc support... using fallback UObject::RepeatString method for allocation!")
#endif
)---";

auto GMallocWrapper_decl = R"---(template <typename T> T getVirtualFunc(const void* instance, size_t index)
{
	auto vtable = *static_cast<const void***>(const_cast<void*>(instance));
	return reinterpret_cast<T>(vtable[index]);
}

// Wrapper for GMalloc
//
// Usage: void* myNewMem = GMalloc.Malloc(100);
class GMallocWrapper
{
	void** m_gmallocAddress = nullptr;

public:
	GMallocWrapper() = default;
	GMallocWrapper(uintptr_t gmallocAddress) : m_gmallocAddress(reinterpret_cast<void**>(gmallocAddress)) {}

private:
	template <typename T> T getVirtualFunc(const void* instance, size_t index)
	{
		auto vtable = *static_cast<const void***>(const_cast<void*>(instance));
		return reinterpret_cast<T>(vtable[index]);
	}

public:
	// always dereference GMalloc to get the current FMalloc instance
	void* getFMallocInstance() const { return m_gmallocAddress ? *m_gmallocAddress : nullptr; }

	void* Malloc(DWORD Count)
	{
		void* fmallocInstance = getFMallocInstance();
		if (!fmallocInstance || Count == 0)
			return nullptr;

		using Malloc_t = void*(__thiscall*)(void*, DWORD);
		return getVirtualFunc<Malloc_t>(fmallocInstance, 2)(fmallocInstance, Count);
	}

	void* Realloc(void* Original, SIZE_T Count)
	{
		void* fmallocInstance = getFMallocInstance();
		if (!fmallocInstance || Count == 0)
			return nullptr;

		using Realloc_t = void*(__thiscall*)(void*, void*, SIZE_T);
		return getVirtualFunc<Realloc_t>(fmallocInstance, 3)(fmallocInstance, Original, Count);
	}

	void Free(void* Original)
	{
		void* fmallocInstance = getFMallocInstance();
		if (!fmallocInstance || !Original)
			return;

		using Free_t = void(__thiscall*)(void*, void*);
		getVirtualFunc<Free_t>(fmallocInstance, 4)(fmallocInstance, Original);
	}
};
)---";

auto FStringAddons = R"---(FString FString::create(const FString& old) { return UObject::RepeatString(old, 1); }

#ifndef USE_GMALLOC
FString FString::create(const std::string& str)
{
	std::wstring wideStr = StringUtils::ToWideString(str);
	return UObject::RepeatString(wideStr.data(), 1);
}
#else
FString FString::create(const std::string& str)
{
	FString      out;
	std::wstring text = StringUtils::ToWideString(str);
	if (text.empty())
		return out;

	size_t len   = text.size();
	size_t bytes = (len + 1) * sizeof(wchar_t);
	auto*  data  = static_cast<wchar_t*>(GMalloc.Malloc(static_cast<SIZE_T>(bytes)));
	if (!data)
		return out;

	memcpy(data, text.c_str(), bytes);
	out.ArrayData  = data;
	out.ArrayCount = static_cast<int32_t>(len + 1);
	out.ArrayMax   = out.ArrayCount;
	return out;
}
#endif
)---";

auto StringUtils = R"---(namespace StringUtils
{
	inline std::string ToString(const std::wstring& str)
	{
		if (str.empty())
            return "";
		int32_t size = WideCharToMultiByte(CP_UTF8, 0, str.data(), -1, nullptr, 0, nullptr, nullptr);
		if (size <= 0)
            return "";
		std::string return_str(size - 1, 0);
		WideCharToMultiByte(CP_UTF8, 0, str.data(), -1, const_cast<char*>(return_str.data()), size, nullptr, nullptr);
		return return_str;
	}

	inline std::wstring ToWideString(const std::string& str)
	{
		if (str.empty())
            return L"";
		int32_t size = MultiByteToWideChar(CP_UTF8, 0, str.data(), -1, nullptr, 0);
		if (size <= 0)
            return L"";
		std::wstring return_str(size - 1, 0);
		MultiByteToWideChar(CP_UTF8, 0, str.data(), -1, const_cast<wchar_t*>(return_str.data()), size);
		return return_str;
	}
}
)---";

auto TArrayUtils_decl = R"---(#ifndef USE_GMALLOC
namespace TArrayUtils
{
struct TArrayBase
{
	void*   data;
	int32_t size;
	int32_t capacity;
};

bool extendCapacity(void* inOldTArray, int32_t newCapacity, void* outNewTArray, int32_t elementSize);
} // namespace TArrayUtils
#endif
)---";

auto TArrayUtils = R"---(#ifndef USE_GMALLOC
namespace TArrayUtils
{
bool extendCapacity(void* inOldTArray, int32_t newCapacity, void* outNewTArray, int32_t elementSize)
{
	if (inOldTArray == outNewTArray)
		throw std::logic_error("extendCapacity: input and output TArray pointers must not alias");

	auto*        oldArray    = reinterpret_cast<TArrayBase*>(inOldTArray);
	auto*        newArray    = reinterpret_cast<TArrayBase*>(outNewTArray);
	const size_t newByteSize = static_cast<size_t>(newCapacity) * elementSize;

	if (!oldArray || !newArray || elementSize == 0 || newCapacity <= 0 || newCapacity <= oldArray->capacity)
		return false;

	// use UObject::RepeatString to allocate buffer using UE
	const size_t requiredWChars = (newByteSize + 1) / sizeof(wchar_t);                     // +1 to force ceiling
	FString      fstr = UObject::RepeatString(L"A", static_cast<int32_t>(requiredWChars)); // actually allocates requiredWChars + 1
	                                                                                       // (for '\0')... but thats fine

	// verify buffer size
	const size_t allocatedBytes = fstr.size() * sizeof(wchar_t);
	if (allocatedBytes < newByteSize)
		return false;

	// copy over data from old TArray
	void* newData = *reinterpret_cast<void**>(&fstr);                   // yoink internal buffer from FString
	ZeroMemory(newData, allocatedBytes);                                // zero it out
	std::memcpy(newData, oldArray->data, oldArray->size * elementSize); // copy existing data to newly created buffer

	// fill in member values for new TArray
	newArray->data     = newData;
	newArray->size     = oldArray->size;
	newArray->capacity = newCapacity;

	// set FString's data member (wchar_t*) to be nullptr, to prevent UE's FString destructor from trying to free the memory
	*reinterpret_cast<void**>(&fstr) = nullptr;
	return true;
}
} // namespace TArrayUtils
#endif
)---";

auto UE_Extras_decl_1 = R"---(inline bool validUObject(const UObject* obj)
{
	return obj && !(obj->ObjectFlags & RF_BadObjectFlags);
}

inline bool sameId(const FUniqueNetId& left, const FUniqueNetId& right)
{
	return (left.EpicAccountId == right.EpicAccountId) && (left.Uid == right.Uid);
}
)---";

// cant use raw string literal here bc it's too big for MSVC's string literal char limit (~16,384 characters.. 16KB)... smh wow
auto UE_Extras_decl_2 =
    "typedef float         FLOAT;\n"
    "typedef uint8_t       BYTE;     // 8-bit  unsigned.\n"
    "typedef int32_t       INT;      // 32-bit signed.\n"
    "typedef uint32_t      UINT;     // 32-bit unsigned.\n"
    "typedef UINT          UBOOL;    // Boolean 0 (false) or 1 (true).\n"
    "typedef unsigned long BITFIELD; // For bitfields.\n"
    "typedef double        DOUBLE;   // 64-bit IEEE double.\n"
    "\n"
    "// TLinkedList:\n"
    "// https://github.com/CodeRedModding/UnrealEngine3/blob/7bf53e29f620b0d4ca5c9bd063a2d2dbcee732fe/Development/Src/Core/Inc/List.h#L18\n"
    "template <class ElementType> class TLinkedList\n"
    "{\n"
    "public:\n"
    "\t/**\n"
    "\t * Used to iterate over the elements of a linked list.\n"
    "\t */\n"
    "\tclass TIterator\n"
    "\t{\n"
    "\tprivate:\n"
    "\t\t// private class for safe bool conversion\n"
    "\t\tstruct PrivateBooleanHelper\n"
    "\t\t{\n"
    "\t\t\tINT Value;\n"
    "\t\t};\n"
    "\n"
    "\tpublic:\n"
    "\t\tTIterator(TLinkedList* FirstLink) : CurrentLink(FirstLink) {}\n"
    "\n"
    "\t\t/**\n"
    "\t\t * Advances the iterator to the next element.\n"
    "\t\t */\n"
    "\t\tvoid Next()\n"
    "\t\t{\n"
    "\t\t\tcheckSlow(CurrentLink);\n"
    "\t\t\tCurrentLink = CurrentLink->NextLink;\n"
    "\t\t}\n"
    "\n"
    "\t\tTIterator& operator++()\n"
    "\t\t{\n"
    "\t\t\tNext();\n"
    "\t\t\treturn *this;\n"
    "\t\t}\n"
    "\n"
    "\t\tTIterator operator++(int)\n"
    "\t\t{\n"
    "\t\t\tTIterator Tmp(*this);\n"
    "\t\t\tNext();\n"
    "\t\t\treturn Tmp;\n"
    "\t\t}\n"
    "\n"
    "\t\t/** conversion to \"bool\" returning TRUE if the iterator is valid. */\n"
    "\t\ttypedef bool PrivateBooleanType;\n"
    "\t\toperator PrivateBooleanType() const { return CurrentLink != NULL ? &PrivateBooleanHelper::Value : NULL; }\n"
    "\t\tbool operator!() const { return !operator PrivateBooleanType(); }\n"
    "\n"
    "\t\t// Accessors.\n"
    "\t\tElementType& operator->() const\n"
    "\t\t{\n"
    "\t\t\tcheckSlow(CurrentLink);\n"
    "\t\t\treturn CurrentLink->Element;\n"
    "\t\t}\n"
    "\t\tElementType& operator*() const\n"
    "\t\t{\n"
    "\t\t\tcheckSlow(CurrentLink);\n"
    "\t\t\treturn CurrentLink->Element;\n"
    "\t\t}\n"
    "\n"
    "\tprivate:\n"
    "\t\tTLinkedList* CurrentLink;\n"
    "\t};\n"
    "\n"
    "\t// Constructors.\n"
    "\tTLinkedList() : NextLink(NULL), PrevLink(NULL) {}\n"
    "\tTLinkedList(const ElementType& InElement) : Element(InElement), NextLink(NULL), PrevLink(NULL) {}\n"
    "\n"
    "\t/**\n"
    "\t * Removes this element from the list in constant time.\n"
    "\t *\n"
    "\t * This function is safe to call even if the element is not linked.\n"
    "\t */\n"
    "\tvoid Unlink()\n"
    "\t{\n"
    "\t\tif (NextLink)\n"
    "\t\t{\n"
    "\t\t\tNextLink->PrevLink = PrevLink;\n"
    "\t\t}\n"
    "\t\tif (PrevLink)\n"
    "\t\t{\n"
    "\t\t\t*PrevLink = NextLink;\n"
    "\t\t}\n"
    "\t\t// Make it safe to call Unlink again.\n"
    "\t\tNextLink = NULL;\n"
    "\t\tPrevLink = NULL;\n"
    "\t}\n"
    "\n"
    "\t/**\n"
    "\t * Adds this element to a list, before the given element.\n"
    "\t *\n"
    "\t * @param Before\tThe link to insert this element before.\n"
    "\t */\n"
    "\tvoid Link(TLinkedList*& Before)\n"
    "\t{\n"
    "\t\tif (Before)\n"
    "\t\t{\n"
    "\t\t\tBefore->PrevLink = &NextLink;\n"
    "\t\t}\n"
    "\t\tNextLink = Before;\n"
    "\t\tPrevLink = &Before;\n"
    "\t\tBefore   = this;\n"
    "\t}\n"
    "\n"
    "\t/**\n"
    "\t * Returns whether element is currently linked.\n"
    "\t *\n"
    "\t * @return TRUE if currently linked, FALSE othwerise\n"
    "\t */\n"
    "\tbool IsLinked() { return PrevLink != NULL; }\n"
    "\n"
    "\t// Accessors.\n"
    "\tElementType&       operator->() { return Element; }\n"
    "\tconst ElementType& operator->() const { return Element; }\n"
    "\tElementType&       operator*() { return Element; }\n"
    "\tconst ElementType& operator*() const { return Element; }\n"
    "\tTLinkedList*       Next() { return NextLink; }\n"
    "\n"
    "private:\n"
    "\tElementType   Element;\n"
    "\tTLinkedList*  NextLink;\n"
    "\tTLinkedList** PrevLink;\n"
    "};\n"
    "\n"
    "// FMipBiasFade:\n"
    "// "
    "https://github.com/CodeRedModding/UnrealEngine3/blob/7bf53e29f620b0d4ca5c9bd063a2d2dbcee732fe/Development/Src/Engine/Inc/"
    "UnRenderUtils.h#L611\n"
    "struct FMipBiasFade\n"
    "{\n"
    "\tFLOAT TotalMipCount;      // Number of mip - levels in the texture.\n"
    "\tFLOAT MipCountDelta;      // Number of mip-levels to fade (negative if fading out / decreasing the mipcount).\n"
    "\tFLOAT StartTime;          // Timestamp when the fade was started.\n"
    "\tFLOAT MipCountFadingRate; // Number of seconds to interpolate through all MipCountDelta (inverted).\n"
    "\tFLOAT BiasOffset;         // Difference between total texture mipcount and the starting mipcount for the fade.\n"
    "};\n"
    "\n"
    "// EShaderFrequency:\n"
    "// "
    "https://github.com/CodeRedModding/UnrealEngine3/blob/7bf53e29f620b0d4ca5c9bd063a2d2dbcee732fe/Development/Src/Engine/Inc/"
    "ShaderCompiler.h#L9\n"
    "enum EShaderFrequency\n"
    "{\n"
    "\tSF_Vertex         = 0,\n"
    "\tSF_Hull           = 1,\n"
    "\tSF_Domain         = 2,\n"
    "\tSF_Pixel          = 3,\n"
    "\tSF_Geometry       = 4,\n"
    "\tSF_Compute        = 5,\n"
    "\tSF_NumFrequencies = 6,\n"
    "};\n"
    "\n"
    "// forward declarations to make it compile (make sure to include \"d3d11.h\" before this file if you plan on using these\n"
    "// types)\n"
    "class FD3D11DynamicRHI;\n"
    "class ID3D11ShaderResourceView;\n"
    "class ID3D11RenderTargetView;\n"
    "class ID3D11DepthStencilView;\n"
    "class ID3D11Texture2D;\n"
    "\n"
    "// FD3D11Texture2D:\n"
    "// "
    "https://github.com/CodeRedModding/UnrealEngine3/blob/7bf53e29f620b0d4ca5c9bd063a2d2dbcee732fe/Development/Src/D3D11Drv/Inc/"
    "D3D11Resources.h#L327\n"
    "struct FD3D11Texture2D\n"
    "{\n"
    "\tID3D11ShaderResourceView* View;\n"
    "\tID3D11ShaderResourceView* ViewLinear;\n"
    "\tID3D11RenderTargetView*   RenderTargetView;\n"
    "\tID3D11DepthStencilView*   DepthStencilView;\n"
    "\tTArray<INT>               BoundShaderResourceSlots[SF_NumFrequencies];\n"
    "\tconst UINT                SizeX;\n"
    "\tconst UINT                SizeY;\n"
    "\tconst UINT                SizeZ;\n"
    "\tconst UINT                NumMips;\n"
    "\tEPixelFormat              Format;\n"
    "\tFD3D11DynamicRHI*         D3DRHI;\n"
    "\tINT                       MemorySize;\n"
    "\tID3D11Texture2D*          Resource;\n"
    "\tconst BITFIELD            bCubemap : 1;\n"
    "\tUINT                      Flags;\n"
    "\n"
    "\tFORCEINLINE ID3D11ShaderResourceView* GetShaderResourceView() { return View; }\n"
    "\tvirtual void                          DummyForVptr() {}\n"
    "}; // Size: 0x00C0\n"
    "\n"
    "// FRenderResource:\n"
    "// https://github.com/CodeRedModding/UnrealEngine3/blob/main/Development/Src/Engine/Inc/RenderResource.h#L9\n"
    "struct FRenderResource\n"
    "{\n"
    "\tTLinkedList<FRenderResource*>* ResourceLink; // was previously a TLinkedList class (not pointer) before RL update v2.54 (7/29/25)\n"
    "\tBITFIELD                       bInitialized : 1;\n"
    "\n"
    "\tvirtual ~FRenderResource() {}\n"
    "};\n"
    "\n"
    "// FTexture:\n"
    "// https://github.com/CodeRedModding/UnrealEngine3/blob/main/Development/Src/Engine/Inc/RenderResource.h#L127\n"
    "struct FTexture : public FRenderResource\n"
    "{\n"
    "\tFD3D11Texture2D* TextureRHI;       // The texture's RHI resource.\n"
    "\tvoid*            SamplerStateRHI;  // The sampler state to use for the texture.\n"
    "\tmutable DOUBLE   LastRenderTime;   // The last time the texture has been bound\n"
    "\tFMipBiasFade     MipBiasFade;      // Base values for fading in/out mip-levels.\n"
    "\tUBOOL            bGreyScaleFormat; // TRUE if the texture is in a greyscale texture format.\n"
    "\n"
    "\t/**\n"
    "\t * TRUE if the texture is in the same gamma space as the intended rendertarget (e.g. screenshots).\n"
    "\t * The texture will have sRGB==FALSE and bIgnoreGammaConversions==TRUE, causing a non-sRGB texture lookup\n"
    "\t * and no gamma-correction in the shader.\n"
    "\t */\n"
    "\tUBOOL bIgnoreGammaConversions;\n"
    "};\n"
    "\n"
    "// As a member of UTexture:\n"
    "// https://github.com/CodeRedModding/UnrealEngine3/blob/main/Development/Src/Engine/Inc/EngineTextureClasses.h#L380\n"
    "//\n"
    "// FTextureResource:\n"
    "// https://github.com/CodeRedModding/UnrealEngine3/blob/main/Development/Src/Engine/Inc/UnTex.h#L33\n"
    "struct FTextureResource : public FTexture\n"
    "{\n"
    "\tFRenderCommandFence ReleaseFence; // 0x0050\n"
    "}; // Size: 0x0058\n";

auto UE_Extras = "";
} // namespace ExtraPiecesOfCode

namespace PiecesOfCode
{
const std::string TArray_Iterator = R"---(template <typename TArray> class TIterator
{
public:
	using ElementType           = typename TArray::ElementType;
	using ElementPointer        = ElementType*;
	using ElementReference      = ElementType&;
	using ElementConstReference = const ElementType&;

private:
	ElementPointer IteratorData;

public:
	TIterator(ElementPointer inElementPointer) : IteratorData(inElementPointer) {}
	~TIterator() {}

public:
	TIterator& operator++()
	{
		IteratorData++;
		return *this;
	}

	TIterator operator++(int32_t)
	{
		TIterator iteratorCopy = *this;
		++(*this);
		return iteratorCopy;
	}

	TIterator& operator--()
	{
		IteratorData--;
		return *this;
	}

	TIterator operator--(int32_t)
	{
		TIterator iteratorCopy = *this;
		--(*this);
		return iteratorCopy;
	}

	ElementReference operator[](int32_t index) { return *(IteratorData[index]); }
	ElementPointer   operator->() { return IteratorData; }
	ElementReference operator*() { return *IteratorData; }

public:
	bool operator==(const TIterator& other) const { return (IteratorData == other.IteratorData); }
	bool operator!=(const TIterator& other) const { return !(*this == other); }
};
)---";

const std::string TArray_Class = R"---(template <typename InElementType> class TArray
{
public:
	using ElementType           = InElementType;
	using ElementPointer        = ElementType*;
	using ElementReference      = ElementType&;
	using ElementConstPointer   = const ElementType*;
	using ElementConstReference = const ElementType&;
	using Iterator              = TIterator<TArray<ElementType>>;

private:
	ElementPointer m_data;
	int32_t        m_size;
	int32_t        m_capacity;

public:
	TArray() : m_data(nullptr), m_size(0), m_capacity(0) {}
	~TArray() { clear(); }

public:
	ElementConstReference operator[](int32_t index) const { return m_data[index]; }
	ElementReference      operator[](int32_t index) { return m_data[index]; }
	ElementConstReference at(int32_t index) const { return m_data[index]; }
	ElementReference      at(int32_t index) { return m_data[index]; }
	ElementConstPointer   data() const { return m_data; }

	void push_back(ElementConstReference newElement)
	{
		if (m_size >= m_capacity)
			ReAllocate(m_size + 1);

		new (&m_data[m_size]) ElementType(newElement);
		m_size++;
	}

	void push_back(ElementType&& newElement)
	{
		if (m_size >= m_capacity)
			ReAllocate(m_size + 1);

		new (&m_data[m_size]) ElementType(std::move(newElement));
		m_size++;
	}

	void pop_back()
	{
		if (m_size <= 0)
			return;

		m_size--;
		DestroyElement(&m_data[m_size]);
	}

	void clear()
	{
		if constexpr (!std::is_trivially_destructible_v<ElementType>)
		{
			for (int32_t i = 0; i < m_size; ++i)
				DestroyElement(&m_data[i]);
		}

		m_size = 0;
	}

	int32_t size() const { return m_size; }
	int32_t capacity() const { return m_capacity; }

	bool empty() const
	{
		if (m_data)
			return (size() == 0);

		return true;
	}

	Iterator begin() const { return Iterator(m_data); }
	Iterator end() const { return Iterator(m_data + m_size); }

private:
	static void DestroyElement(ElementPointer element)
	{
		if constexpr (!std::is_trivially_destructible_v<ElementType>)
			element->~ElementType();
	}

	void ReAllocate(int32_t newArrayMax)
	{
#ifdef USE_GMALLOC
		if (newArrayMax <= m_capacity)
			return; // nothing to do

		const size_t newByteSize = static_cast<size_t>(newArrayMax) * sizeof(ElementType);
		const size_t oldByteSize = static_cast<size_t>(m_capacity) * sizeof(ElementType);

		void* newData = GMalloc.Realloc(m_data, newByteSize);
		if (!newData)
			return; // allocation failed (ideally handle more gracefully)

		// zero the newly allocated tail to avoid uninitialized elements
		if (newArrayMax > m_capacity)
			std::memset(static_cast<uint8_t*>(newData) + oldByteSize, 0, newByteSize - oldByteSize);

		m_data     = reinterpret_cast<ElementPointer>(newData);
		m_capacity = newArrayMax;
#else
		if (newArrayMax <= m_capacity) // only reallocate if we're growing the cacpacity, shrinking would be kinda pointless
			return;

		TArrayUtils::TArrayBase tempArray{};
		if (TArrayUtils::extendCapacity(this, newArrayMax, &tempArray, sizeof(ElementType)))
		{
			m_data     = reinterpret_cast<ElementPointer>(tempArray.data);
			m_size     = tempArray.size;
			m_capacity = tempArray.capacity;
		}
#endif
	}
};
)---";

const std::string TMap_Class = R"---(template <typename TKey, typename TValue> class TMap
{
private:
	struct TPair
	{
		TKey   Key;
		TValue Value;
	};

public:
	using ElementType           = TPair;
	using ElementPointer        = ElementType*;
	using ElementReference      = ElementType&;
	using ElementConstReference = const ElementType&;
	using Iterator              = TIterator<class TArray<ElementType>>;

public:
	class TArray<ElementType> Elements;        // 0x0000 (0x0010)
	struct FPointer           IndirectData;    // 0x0010 (0x0008)
	int32_t                   InlineData[0x4]; // 0x0018 (0x0010)
	int32_t                   NumBits;         // 0x0028 (0x0004)
	int32_t                   MaxBits;         // 0x002C (0x0004)
	int32_t                   FirstFreeIndex;  // 0x0030 (0x0004)
	int32_t                   NumFreeIndices;  // 0x0034 (0x0004)
	int64_t                   InlineHash;      // 0x0038 (0x0008)
	int32_t*                  Hash;            // 0x0040 (0x0008)
	int32_t                   HashCount;       // 0x0048 (0x0004)
public:
	TMap() : IndirectData(NULL), NumBits(0), MaxBits(0), FirstFreeIndex(0), NumFreeIndices(0), InlineHash(0), Hash(nullptr), HashCount(0) {}

	TMap(struct FMap_Mirror& other)
	    : IndirectData(NULL), NumBits(0), MaxBits(0), FirstFreeIndex(0), NumFreeIndices(0), InlineHash(0), Hash(nullptr), HashCount(0)
	{
		assign(other);
	}

	TMap(const TMap<TKey, TValue>& other)
	    : IndirectData(NULL), NumBits(0), MaxBits(0), FirstFreeIndex(0), NumFreeIndices(0), InlineHash(0), Hash(nullptr), HashCount(0)
	{
		assign(other);
	}

	~TMap() {}

public:
	TMap<TKey, TValue>& assign(struct FMap_Mirror& other)
	{
		*this = *reinterpret_cast<TMap<TKey, TValue>*>(&other);
		return *this;
	}

	TMap<TKey, TValue>& assign(const TMap<TKey, TValue>& other)
	{
		Elements       = other.Elements;
		IndirectData   = other.IndirectData;
		InlineData[0]  = other.InlineData[0];
		InlineData[1]  = other.InlineData[1];
		InlineData[2]  = other.InlineData[2];
		InlineData[3]  = other.InlineData[3];
		NumBits        = other.NumBits;
		MaxBits        = other.MaxBits;
		FirstFreeIndex = other.FirstFreeIndex;
		NumFreeIndices = other.NumFreeIndices;
		InlineHash     = other.InlineHash;
		Hash           = other.Hash;
		HashCount      = other.HashCount;
		return *this;
	}

	TValue& at(const TKey& key)
	{
		for (TPair& pair : Elements)
		{
			if (pair.Key != key)
				continue;

			return pair.Value;
		}
	}

	const TValue& at(const TKey& key) const
	{
		for (const TPair& pair : Elements)
		{
			if (pair.Key != key)
				continue;

			return pair.Value;
		}
	}

	TPair&       at_index(int32_t index) { return Elements[index]; }
	const TPair& at_index(int32_t index) const { return Elements[index]; }
	int32_t      size() const { return Elements.size(); }
	int32_t      capacity() const { return Elements.capacity(); }
	bool         empty() const { return Elements.empty(); }
	Iterator     begin() { return Elements.begin(); }
	Iterator     end() { return Elements.end(); }

public:
	TValue&             operator[](const TKey& key) { return at(key); }
	const TValue&       operator[](const TKey& key) const { return at(key); }
	TMap<TKey, TValue>& operator=(const struct FMap_Mirror& other) { return assign(other); }
	TMap<TKey, TValue>& operator=(const TMap<TKey, TValue>& other) { return assign(other); }
};
)---";

const std::string FNameEntry_Struct = R"---(class FNameEntry
{
public:)---";

const std::string FNameEntry_Struct_UTF16 = R"---(
public:
	FNameEntry() : Flags{ 0 }, Index{ -1 }, Name{ 0 }, UnknownData00{ 0 } {}
	~FNameEntry() {}

public:
	uint64_t GetFlags() const
	{
		return Flags;
	}

	int32_t GetIndex() const
	{
		return Index;
	}

	const wchar_t* GetWideName() const
	{
		return Name;
	}

	std::wstring ToWideString() const
	{
		const wchar_t* wideName = GetWideName();
		if (!wideName)
			return L"";

		return std::wstring(wideName);
	}

	std::string ToString() const
	{
		std::wstring wstr = ToWideString();
		return std::string(wstr.begin(), wstr.end());
	}
};
)---";

const std::string FNameEntry_Struct_UTF8 = "\npublic:\n"
                                           "\tFNameEntry() : Flags(0), Index(-1), HashNext(nullptr) {}\n"
                                           "\t~FNameEntry() {}\n"
                                           "\n"
                                           "public:\n"
                                           "\tuint64_t GetFlags() const\n"
                                           "\t{\n"
                                           "\t\treturn Flags;\n"
                                           "\t}\n"
                                           "\n"
                                           "\tint32_t GetIndex() const\n"
                                           "\t{\n"
                                           "\t\treturn Index;\n"
                                           "\t}\n"
                                           "\n"
                                           "\tconst char* GetAnsiName() const\n"
                                           "\t{\n"
                                           "\t\treturn Name;\n"
                                           "\t}\n"
                                           "\n"
                                           "\tstd::string ToString() const\n"
                                           "\t{\n"
                                           "\t\treturn std::string(Name);\n"
                                           "\t}\n"
                                           "};\n";

const std::string FName_Struct_UTF16 =
    R"---(class FName
{
public:
	using ElementType = const wchar_t;
	using ElementPointer = ElementType*;

private:
	int32_t			FNameEntryId;									// 0x0000 (0x04)
	int32_t			InstanceNumber;									// 0x0004 (0x04)

public:
	FName() : FNameEntryId(-1), InstanceNumber(0) {}

	FName(int32_t id) : FNameEntryId(id), InstanceNumber(0) {}

	FName(const ElementPointer nameToFind) : FNameEntryId(-1), InstanceNumber(0)
	{
		static std::vector<int32_t> foundNames{};

		for (int32_t entryId : foundNames)
		{
			if (!Names()->at(entryId))
				continue;

			if (wcscmp(Names()->at(entryId)->Name, nameToFind) == 0)
			{
				FNameEntryId = entryId;
				return;
			}
		}

		for (int32_t i = 0; i < Names()->size(); ++i)
		{
			if (!Names()->at(i))
				continue;

			if (wcscmp(Names()->at(i)->Name, nameToFind) == 0)
			{
				foundNames.push_back(i);
				FNameEntryId = i;
				return;
			}
		}
	}

	FName(const FName& name) : FNameEntryId(name.FNameEntryId), InstanceNumber(name.InstanceNumber) {}

	~FName() {}

public:
	static class TArray<class FNameEntry*>* Names()
	{
		return reinterpret_cast<TArray<FNameEntry*>*>(GNames);
	}

	int32_t GetDisplayIndex() const
	{
		return FNameEntryId;
	}

	const FNameEntry GetDisplayNameEntry() const
	{
		if (!IsValid())
			return FNameEntry();

		return *Names()->at(FNameEntryId);
	}

	FNameEntry* GetEntry()
	{
		if (!IsValid())
			return nullptr;

		return Names()->at(FNameEntryId);
	}

	int32_t GetInstance() const
	{
		return InstanceNumber;
	}

	void SetInstance(int32_t newNumber)
	{
		InstanceNumber = newNumber;
	}

	std::string ToString() const
	{
		if (!IsValid())
			return "UnknownName";
			
		return GetDisplayNameEntry().ToString();
	}

	bool IsValid() const
	{
		return (FNameEntryId >= 0) && (FNameEntryId <= Names()->size());
	}

	static FName find(const std::string& str)
	{
		std::wstring wideStr = StringUtils::ToWideString(str);
		return FName(wideStr.data());
	}

public:
	FName& operator=(const FName& other)
	{
		FNameEntryId = other.FNameEntryId;
		InstanceNumber = other.InstanceNumber;
		return *this;
	}

	bool operator==(const FName& other) const
	{
		return ((FNameEntryId == other.FNameEntryId) && (InstanceNumber == other.InstanceNumber));
	}

	bool operator!=(const FName& other) const
	{
		return !(*this == other);
	}
};
)---";

const std::string FName_Struct_UTF8 =
    "class FName\n"
    "{\n"
    "public:\n"
    "\tusing ElementType = const char;\n"
    "\tusing ElementPointer = ElementType*;\n"
    "\n"
    "private:\n"
    "\tint32_t\t\t\tFNameEntryId;\t\t\t\t\t\t\t\t\t// 0x0000 (0x04)\n"
    "\tint32_t\t\t\tInstanceNumber;\t\t\t\t\t\t\t\t\t// 0x0004 (0x04)\n"
    "\n"
    "public:\n"
    "\tFName() : FNameEntryId(-1), InstanceNumber(0) {}\n"
    "\n"
    "\tFName(int32_t id) : FNameEntryId(id), InstanceNumber(0) {}\n"
    "\n"
    "\tFName(ElementPointer nameToFind) : FNameEntryId(-1), InstanceNumber(0)\n"
    "\t{\n"
    "\t\tstatic std::vector<int32_t> nameCache{};\n"
    "\n"
    "\t\tfor (int32_t entryId : nameCache)\n"
    "\t\t{\n"
    "\t\t\tif (Names()->at(entryId))\n"
    "\t\t\t{\n"
    "\t\t\t\tif (strcmp(Names()->at(entryId)->Name, nameToFind) == 0)\n"
    "\t\t\t\t{\n"
    "\t\t\t\t\tFNameEntryId = entryId;\n"
    "\t\t\t\t\treturn;\n"
    "\t\t\t\t}\n"
    "\t\t\t}\n"
    "\t\t}\n"
    "\n"
    "\t\tfor (int32_t i = 0; i < Names()->size(); i++)\n"
    "\t\t{\n"
    "\t\t\tif (Names()->at(i))\n"
    "\t\t\t{\n"
    "\t\t\t\tif (strcmp(Names()->at(i)->Name, nameToFind) == 0)\n"
    "\t\t\t\t{\n"
    "\t\t\t\t\tnameCache.push_back(i);\n"
    "\t\t\t\t\tFNameEntryId = i;\n"
    "\t\t\t\t}\n"
    "\t\t\t}\n"
    "\t\t}\n"
    "\t}\n"
    "\n"
    "\tFName(const FName& name) : FNameEntryId(name.FNameEntryId), InstanceNumber(name.InstanceNumber) {}\n"
    "\n"
    "\t~FName() {}\n"
    "\n"
    "public:\n"
    "\tstatic TArray<FNameEntry*>* Names()\n"
    "\t{\n"
    "\t\tTArray<FNameEntry*>* recastedArray = reinterpret_cast<TArray<FNameEntry*>*>(GNames);\n"
    "\t\treturn recastedArray;\n"
    "\t}\n"
    "\n"
    "\tint32_t GetDisplayIndex() const\n"
    "\t{\n"
    "\t\treturn FNameEntryId;\n"
    "\t}\n"
    "\n"
    "\tconst FNameEntry GetDisplayNameEntry() const\n"
    "\t{\n"
    "\t\tif (IsValid())\n"
    "\t\t{\n"
    "\t\t\treturn *Names()->at(FNameEntryId);\n"
    "\t\t}\n"
    "\n"
    "\t\treturn FNameEntry();\n"
    "\t}\n"
    "\n"
    "\tFNameEntry* GetEntry()\n"
    "\t{\n"
    "\t\tif (IsValid())\n"
    "\t\t{\n"
    "\t\t\treturn Names()->at(FNameEntryId);\n"
    "\t\t}\n"
    "\n"
    "\t\treturn nullptr;\n"
    "\t}\n"
    "\n"
    "\tint32_t GetInstance() const\n"
    "\t{\n"
    "\t\treturn InstanceNumber;\n"
    "\t}\n"
    "\n"
    "\tvoid SetInstance(int32_t newNumber)\n"
    "\t{\n"
    "\t\tInstanceNumber = newNumber;\n"
    "\t}\n"
    "\n"
    "\tstd::string ToString() const\n"
    "\t{\n"
    "\t\tif (IsValid())\n"
    "\t\t{\n"
    "\t\t\treturn GetDisplayNameEntry().ToString();\n"
    "\t\t}\n"
    "\n"
    "\t\treturn \"UnknownName\";\n"
    "\t}\n"
    "\n"
    "\tbool IsValid() const\n"
    "\t{\n"
    "\t\tif ((FNameEntryId < 0 || FNameEntryId > Names()->size()))\n"
    "\t\t{\n"
    "\t\t\treturn false;\n"
    "\t\t}\n"
    "\n"
    "\t\treturn true;\n"
    "\t}\n"
    "\n"
    "public:\n"
    "\tFName& operator=(const FName& other)\n"
    "\t{\n"
    "\t\tFNameEntryId = other.FNameEntryId;\n"
    "\t\tInstanceNumber = other.InstanceNumber;\n"
    "\t\treturn *this;\n"
    "\t}\n"
    "\n"
    "\tbool operator==(const FName& other) const\n"
    "\t{\n"
    "\t\treturn ((FNameEntryId == other.FNameEntryId) && (InstanceNumber == other.InstanceNumber));\n"
    "\t}\n"
    "\n"
    "\tbool operator!=(const FName& other) const\n"
    "\t{\n"
    "\t\treturn !(*this == other);\n"
    "\t}\n"
    "};\n";

const std::string FString_Class_UTF16 = R"---(class FString
{
public:
	using ElementType    = const wchar_t;
	using ElementPointer = ElementType*;

private:
	ElementPointer ArrayData;  // 0x0000 (0x08)
	int32_t        ArrayCount; // 0x0008 (0x04)
	int32_t        ArrayMax;   // 0x000C (0x04)

public:
	FString() : ArrayData(nullptr), ArrayCount(0), ArrayMax(0) {}
	FString(ElementPointer other) : ArrayData(nullptr), ArrayCount(0), ArrayMax(0) { assign(other); }
	~FString() {}

public:
	FString& assign(ElementPointer other)
	{
		ArrayCount = (other ? (wcslen(other) + 1) : 0);
		ArrayMax   = ArrayCount;
		ArrayData  = (ArrayCount > 0 ? other : nullptr);
		return *this;
	}

	FString& assign(const std::wstring& other) { return assign(other.c_str()); }
	FString& assign(const std::string& other) { return assign(StringUtils::ToWideString(other)); }

	std::wstring ToWideString() const
	{
		if (empty() || !isValid())
			return L"";

		return c_str();
	}

	std::string ToString() const
	{
		if (empty() || !isValid())
			return "";

		return StringUtils::ToString(c_str());
	}

	ElementPointer c_str() const { return ArrayData; }

	bool empty() const
	{
		if (!ArrayData)
			return true;

		return (ArrayCount == 0);
	}

	int32_t length() const { return ArrayCount; }
	int32_t size() const { return ArrayMax; }
	bool    isValid() const { return ArrayData && (ArrayCount >= 0) && (ArrayMax >= 0) && (ArrayCount <= ArrayMax); }

	static FString create(const std::string& str);
	static FString create(const FString& old);

public:
	FString& operator=(ElementPointer other) { return assign(other); }
	FString& operator=(const FString& other) { return assign(other.c_str()); }

	bool operator==(const FString& other) const
	{
		if (ArrayCount != other.ArrayCount)
			return false;

		if (!isValid() || !other.isValid())
			return false;

		size_t len = ArrayCount > 0 ? static_cast<size_t>(ArrayCount - 1) : 0;
		return wcsncmp(ArrayData, other.ArrayData, len) == 0;
	}

	bool operator!=(const FString& other) const { return !(*this == other); }
};
)---";

const std::string FString_Class_UTF8 =
    "class FString\n"
    "{\n"
    "public:\n"
    "\tusing ElementType = const char;\n"
    "\tusing ElementPointer = ElementType*;\n"
    "\n"
    "private:\n"
    "\tElementPointer\tArrayData;\t\t\t\t\t\t\t\t\t\t// 0x0000 (0x08)\n"
    "\tint32_t\t\t\tArrayCount;\t\t\t\t\t\t\t\t\t\t// 0x0008 (0x04)\n"
    "\tint32_t\t\t\tArrayMax;\t\t\t\t\t\t\t\t\t\t// 0x000C (0x04)\n"
    "\n"
    "public:\n"
    "\tFString() : ArrayData(nullptr), ArrayCount(0), ArrayMax(0) {}\n"
    "\n"
    "\tFString(ElementPointer other) : ArrayData(nullptr), ArrayCount(0), ArrayMax(0) { assign(other); }\n"
    "\n"
    "\t~FString() {}\n"
    "\n"
    "public:\n"
    "\tFString& assign(ElementPointer other)\n"
    "\t{\n"
    "\t\tArrayCount = (other ? (strlen(other) + 1) : 0);\n"
    "\t\tArrayMax = ArrayCount;\n"
    "\t\tArrayData = (ArrayCount > 0 ? other : nullptr);\n"
    "\t\treturn *this;\n"
    "\t}\n"
    "\n"
    "\tstd::string ToString() const\n"
    "\t{\n"
    "\t\tif (!empty())\n"
    "\t\t{\n"
    "\t\t\treturn std::string(c_str());\n"
    "\t\t}\n"
    "\n"
    "\t\treturn \"\";\n"
    "\t}\n"
    "\n"
    "\tElementPointer c_str() const\n"
    "\t{\n"
    "\t\treturn ArrayData;\n"
    "\t}\n"
    "\n"
    "\tbool empty() const\n"
    "\t{\n"
    "\t\tif (ArrayData)\n"
    "\t\t{\n"
    "\t\t\treturn (ArrayCount == 0);\n"
    "\t\t}\n"
    "\n"
    "\t\treturn true;\n"
    "\t}\n"
    "\n"
    "\tint32_t length() const\n"
    "\t{\n"
    "\t\treturn ArrayCount;\n"
    "\t}\n"
    "\n"
    "\tint32_t size() const\n"
    "\t{\n"
    "\t\treturn ArrayMax;\n"
    "\t}\n"
    "\n"
    "public:\n"
    "\tFString& operator=(ElementPointer other)\n"
    "\t{\n"
    "\t\treturn assign(other);\n"
    "\t}\n"
    "\n"
    "\tFString& operator=(const FString& other)\n"
    "\t{\n"
    "\t\treturn assign(other.c_str());\n"
    "\t}\n"
    "\n"
    "\tbool operator==(const FString& other)\n"
    "\t{\n"
    "\t\treturn (strcmp(ArrayData, other.ArrayData) == 0);\n"
    "\t}\n"
    "\n"
    "\tbool operator!=(const FString& other)\n"
    "\t{\n"
    "\t\treturn (strcmp(ArrayData, other.ArrayData) != 0);\n"
    "\t}\n"
    "};\n";

const std::string FScriptDelegate_Struct = R"---(struct FScriptDelegate
{
	class UObject* Object;
	uint8_t        padding[0x10];
};
)---";

const std::string FPointer_Struct = R"---(struct FPointer
{
	uintptr_t Dummy; // 0x0000 (0x08)
};
)---";

const std::string FQWord_Struct = R"---(struct FQWord
{
	int32_t A; // 0x0000 (0x04)
	int32_t B; // 0x0004 (0x04)
};
)---";

const std::string UObject_FunctionDescriptions = R"---(	static class TArray<class UObject*>* GObjObjects();

	std::string GetName();
	std::string GetNameCPP();
	std::string GetFullName();
	class UObject* GetPackageObj();
	template<typename T> static T* FindObject(const std::string& objectFullName)
	{
		for (UObject* uObject : *UObject::GObjObjects())
		{
			if (uObject && uObject->IsA<T>())
			{
				if (uObject->GetFullName() == objectFullName)
				{
					return reinterpret_cast<T*>(uObject);
				}
			}
		}

		return nullptr;
	}
	static class UClass* FindClass(const std::string& classFullName);
	bool IsA(class UClass* uClass);
	bool IsA(int32_t objInternalInteger);
	template<typename T> bool IsA()
	{
		return IsA(T::StaticClass());
	}

)---";

const std::string UObject_Functions =
    R"---(class TArray<class UObject*>* UObject::GObjObjects() { return reinterpret_cast<TArray<UObject*>*>(GObjects); }

std::string UObject::GetName() { return this->Name.ToString(); }

std::string UObject::GetNameCPP()
{
	std::string nameCPP;

	if (this->IsA<UClass>())
	{
		UClass* uClass = reinterpret_cast<UClass*>(this);

		while (uClass)
		{
			std::string className = uClass->GetName();

			if (className == "Actor")
			{
				nameCPP += "A";
				break;
			}
			else if (className == "Object")
			{
				nameCPP += "U";
				break;
			}

			uClass = reinterpret_cast<UClass*>(uClass->SuperField);
		}
	}
	else
	{
		nameCPP += "F";
	}

	nameCPP += this->GetName();

	return nameCPP;
}

std::string UObject::GetFullName()
{
	std::string fullName = this->GetName();

	for (UObject* uOuter = this->Outer; uOuter; uOuter = uOuter->Outer)
		fullName = (uOuter->GetName() + "." + fullName);

	fullName = (this->Class->GetName() + " " + fullName);
	return fullName;
}

class UObject* UObject::GetPackageObj()
{
	UObject* uPackage = nullptr;
	for (UObject* uOuter = this->Outer; uOuter; uOuter = uOuter->Outer)
		uPackage = uOuter;

	return uPackage;
}

class UClass* UObject::FindClass(const std::string& classFullName)
{
	static bool                                     classesAreCached = false;
	static std::unordered_map<std::string, UClass*> classCache;

	if (!classesAreCached)
	{
		for (UObject* uObject : *UObject::GObjObjects())
		{
			if (!validUObject(uObject))
				continue;

			std::string objFullName = uObject->GetFullName();
			if (objFullName.find("Class") != 0)
				continue;

			classCache[objFullName] = reinterpret_cast<UClass*>(uObject);
		}

		classesAreCached = true;
	}

	if (auto it = classCache.find(classFullName); it != classCache.end())
		return it->second;

	return nullptr;
}

bool UObject::IsA(class UClass* uClass)
{
	for (UClass* uSuperClass = reinterpret_cast<UClass*>(this->Class); uSuperClass;
	    uSuperClass          = reinterpret_cast<UClass*>(uSuperClass->SuperField))
	{
		if (uSuperClass == uClass)
			return true;
	}

	return false;
}

bool UObject::IsA(int32_t objInternalInteger)
{
	UClass* uClass = reinterpret_cast<UClass*>(UObject::GObjObjects()->at(objInternalInteger)->Class);
	if (!uClass)
		return false;

	return this->IsA(uClass);
}

)---";

const std::string UFunction_Functions = R"---(class UFunction* UFunction::FindFunction(const std::string& functionFullName)
{
	static bool                                        initialized = false;
	static std::unordered_map<std::string, UFunction*> functionCache;

	// cache all functions the first time FindFunction is called (can also be done in mod's initialization, where game stutters are
	// expected)
	if (!initialized)
	{
		for (UObject* uObject : *UObject::GObjObjects())
		{
			if (!uObject || !uObject->IsA<UFunction>())
				continue;

			functionCache[uObject->GetFullName()] = static_cast<UFunction*>(uObject);
		}

		initialized = true;
	}

	if (auto it = functionCache.find(functionFullName); it != functionCache.end())
		return it->second;

	return nullptr;
}

)---";

// cant use raw string literal here bc it's too big for MSVC's string literal char limit (~16,384 characters.. 16KB)... smh wow
const std::string EEnumFlags =
    "// https://github.com/CodeRedModding/UnrealEngine3/blob/main/Development/Src/Core/Inc/UnStack.h#L48\n"
    "// State Flags\n"
    "enum EStateFlags\n"
    "{\n"
    "\tSTATE_Editable =\t\t\t\t\t\t0x00000001,\t// State should be user-selectable in UnrealEd.\n"
    "\tSTATE_Auto =\t\t\t\t\t\t\t0x00000002,\t// State is automatic (the default state).\n"
    "\tSTATE_Simulated =\t\t\t\t\t\t0x00000004, // State executes on client side.\n"
    "\tSTATE_HasLocals =\t\t\t\t\t\t0x00000008,\t// State has local variables.\n"
    "};\n"
    "\n"
    "// https://github.com/CodeRedModding/UnrealEngine3/blob/main/Development/Src/Core/Inc/UnStack.h#L60\n"
    "// Function Flags\n"
    "enum EFunctionFlags : uint64_t\n"
    "{\n"
    "\tFUNC_None =\t\t\t\t\t\t\t\t0x00000000,\n"
    "\tFUNC_Final =\t\t\t\t\t\t\t0x00000001,\n"
    "\tFUNC_Defined =\t\t\t\t\t\t\t0x00000002,\n"
    "\tFUNC_Iterator =\t\t\t\t\t\t\t0x00000004,\n"
    "\tFUNC_Latent =\t\t\t\t\t\t\t0x00000008,\n"
    "\tFUNC_PreOperator =\t\t\t\t\t\t0x00000010,\n"
    "\tFUNC_Singular =\t\t\t\t\t\t\t0x00000020,\n"
    "\tFUNC_Net =\t\t\t\t\t\t\t\t0x00000040,\n"
    "\tFUNC_NetReliable =\t\t\t\t\t\t0x00000080,\n"
    "\tFUNC_Simulated =\t\t\t\t\t\t0x00000100,\n"
    "\tFUNC_Exec =\t\t\t\t\t\t\t\t0x00000200,\n"
    "\tFUNC_Native =\t\t\t\t\t\t\t0x00000400,\n"
    "\tFUNC_Event =\t\t\t\t\t\t\t0x00000800,\n"
    "\tFUNC_Operator =\t\t\t\t\t\t\t0x00001000,\n"
    "\tFUNC_Static =\t\t\t\t\t\t\t0x00002000,\n"
    "\tFUNC_NoExport =\t\t\t\t\t\t\t0x00004000,\n"
    "\tFUNC_OptionalParm =\t\t\t\t\t\t0x00004000,\n"
    "\tFUNC_Const =\t\t\t\t\t\t\t0x00008000,\n"
    "\tFUNC_Invariant =\t\t\t\t\t\t0x00010000,\n"
    "\tFUNC_Public =\t\t\t\t\t\t\t0x00020000,\n"
    "\tFUNC_Private =\t\t\t\t\t\t\t0x00040000,\n"
    "\tFUNC_Protected =\t\t\t\t\t\t0x00080000,\n"
    "\tFUNC_Delegate =\t\t\t\t\t\t\t0x00100000,\n"
    "\tFUNC_NetServer =\t\t\t\t\t\t0x00200000,\n"
    "\tFUNC_HasOutParms =\t\t\t\t\t\t0x00400000,\n"
    "\tFUNC_HasDefaults =\t\t\t\t\t\t0x00800000,\n"
    "\tFUNC_NetClient =\t\t\t\t\t\t0x01000000,\n"
    "\tFUNC_DLLImport =\t\t\t\t\t\t0x02000000,\n"
    "\n"
    "\tFUNC_K2Call =\t\t\t\t\t\t\t0x04000000,\n"
    "\tFUNC_K2Override =\t\t\t\t\t\t0x08000000,\n"
    "\tFUNC_K2Pure =\t\t\t\t\t\t\t0x10000000,\n"
    "\tFUNC_EditorOnly =\t\t\t\t\t\t0x20000000,\n"
    "\tFUNC_Lambda =\t\t\t\t\t\t\t0x40000000,\n"
    "\tFUNC_NetValidate =\t\t\t\t\t\t0x80000000,\n"
    "\n"
    "\tFUNC_FuncInherit =\t\t\t\t\t\t(FUNC_Exec | FUNC_Event),\n"
    "\tFUNC_FuncOverrideMatch =\t\t\t\t(FUNC_Exec | FUNC_Final | FUNC_Latent | FUNC_PreOperator | FUNC_Iterator | FUNC_Static | "
    "FUNC_Public | FUNC_Protected | FUNC_Const),\n"
    "\tFUNC_NetFuncFlags =\t\t\t\t\t\t(FUNC_Net | FUNC_NetReliable | FUNC_NetServer | FUNC_NetClient),\n"
    "\n"
    "\tFUNC_AllFlags =\t\t\t\t\t\t\t0xFFFFFFFF\n"
    "};\n"
    "\n"
    "// https://github.com/CodeRedModding/UnrealEngine3/blob/main/Development/Src/Core/Inc/UnObjBas.h#L238\n"
    "// Proprerty Flags\n"
    "enum EPropertyFlags : uint64_t\n"
    "{\n"
    "\tCPF_Edit =\t\t\t\t\t\t\t\t0x0000000000000001,\t// Property is user-settable in the editor.\n"
    "\tCPF_Const =\t\t\t\t\t\t\t\t0x0000000000000002,\t// Actor\'s property always matches class\'s default actor property.\n"
    "\tCPF_Input =\t\t\t\t\t\t\t\t0x0000000000000004,\t// Variable is writable by the input system.\n"
    "\tCPF_ExportObject =\t\t\t\t\t\t0x0000000000000008,\t// Object can be exported with actor.\n"
    "\tCPF_OptionalParm =\t\t\t\t\t\t0x0000000000000010,\t// Optional parameter (if CPF_Param is set).\n"
    "\tCPF_Net =\t\t\t\t\t\t\t\t0x0000000000000020,\t// Property is relevant to network replication.\n"
    "\tCPF_EditFixedSize =\t\t\t\t\t\t0x0000000000000040, // Indicates that elements of an array can be modified, but its size cannot be "
    "changed.\n"
    "\tCPF_Parm =\t\t\t\t\t\t\t\t0x0000000000000080,\t// Function/When call parameter.\n"
    "\tCPF_OutParm =\t\t\t\t\t\t\t0x0000000000000100,\t// Value is copied out after function call.\n"
    "\tCPF_SkipParm =\t\t\t\t\t\t\t0x0000000000000200,\t// Property is a short-circuitable evaluation function parm.\n"
    "\tCPF_ReturnParm =\t\t\t\t\t\t0x0000000000000400,\t// Return value.\n"
    "\tCPF_CoerceParm =\t\t\t\t\t\t0x0000000000000800,\t// Coerce args into this function parameter.\n"
    "\tCPF_Native =\t\t\t\t\t\t\t0x0000000000001000,\t// Property is native: C++ code is responsible for serializing it.\n"
    "\tCPF_Transient =\t\t\t\t\t\t\t0x0000000000002000,\t// Property is transient: shouldn\'t be saved, zero-filled at load time.\n"
    "\tCPF_Config =\t\t\t\t\t\t\t0x0000000000004000,\t// Property should be loaded/saved as permanent profile.\n"
    "\tCPF_Localized =\t\t\t\t\t\t\t0x0000000000008000,\t// Property should be loaded as localizable text.\n"
    "\tCPF_Travel =\t\t\t\t\t\t\t0x0000000000010000,\t// Property travels across levels/servers.\n"
    "\tCPF_EditConst =\t\t\t\t\t\t\t0x0000000000020000,\t// Property is uneditable in the editor.\n"
    "\tCPF_GlobalConfig =\t\t\t\t\t\t0x0000000000040000,\t// Load config from base class, not subclass.\n"
    "\tCPF_Component =\t\t\t\t\t\t\t0x0000000000080000,\t// Property containts component references.\n"
    "\tCPF_AlwaysInit =\t\t\t\t\t\t0x0000000000100000,\t// Property should never be exported as NoInit(@todo - this doesn\'t need to be a "
    "property flag...only used during make).\n"
    "\tCPF_DuplicateTransient =\t\t\t\t0x0000000000200000, // Property should always be reset to the default value during any type of "
    "duplication (copy/paste, binary duplication, etc.).\n"
    "\tCPF_NeedCtorLink =\t\t\t\t\t\t0x0000000000400000,\t// Fields need construction/destruction.\n"
    "\tCPF_NoExport =\t\t\t\t\t\t\t0x0000000000800000,\t// Property should not be exported to the native class header file.\n"
    "\tCPF_NoClear =\t\t\t\t\t\t\t0x0000000002000000,\t// Hide clear (and browse) button.\n"
    "\tCPF_EditInline =\t\t\t\t\t\t0x0000000004000000,\t// Edit this object reference inline.\t\n"
    "\tCPF_EditInlineUse =\t\t\t\t\t\t0x0000000010000000,\t// EditInline with Use button.\n"
    "\tCPF_EditFindable =\t\t\t\t\t\t0x0000000008000000,\t// References are set by clicking on actors in the editor viewports.\n"
    "\tCPF_Deprecated =\t\t\t\t\t\t0x0000000020000000,\t// Property is deprecated.  Read it from an archive, but don\'t save it.\t\n"
    "\tCPF_DataBinding =\t\t\t\t\t\t0x0000000040000000,\t// Indicates that this property should be exposed to data stores.\n"
    "\tCPF_SerializeText =\t\t\t\t\t\t0x0000000080000000,\t// Native property should be serialized as text (ImportText, ExportText).\n"
    "\tCPF_RepNotify =\t\t\t\t\t\t\t0x0000000100000000,\t// Notify actors when a property is replicated.\n"
    "\tCPF_Interp =\t\t\t\t\t\t\t0x0000000200000000,\t// Interpolatable property for use with matinee.\n"
    "\tCPF_NonTransactional =\t\t\t\t\t0x0000000400000000,\t// Property isn\'t transacted.\n"
    "\tCPF_EditorOnly =\t\t\t\t\t\t0x0000000800000000,\t// Property should only be loaded in the editor.\n"
    "\tCPF_NotForConsole =\t\t\t\t\t\t0x0000001000000000, // Property should not be loaded on console (or be a console cooker "
    "commandlet).\n"
    "\tCPF_RepRetry =\t\t\t\t\t\t\t0x0000002000000000, // Property replication of this property if it fails to be fully sent (e.g. object "
    "references not yet available to serialize over the network).\n"
    "\tCPF_PrivateWrite =\t\t\t\t\t\t0x0000004000000000, // Property is const outside of the class it was declared in.\n"
    "\tCPF_ProtectedWrite =\t\t\t\t\t0x0000008000000000, // Property is const outside of the class it was declared in and subclasses.\n"
    "\tCPF_ArchetypeProperty =\t\t\t\t\t0x0000010000000000, // Property should be ignored by archives which have ArIgnoreArchetypeRef "
    "set.\n"
    "\tCPF_EditHide =\t\t\t\t\t\t\t0x0000020000000000, // Property should never be shown in a properties window.\n"
    "\tCPF_EditTextBox =\t\t\t\t\t\t0x0000040000000000, // Property can be edited using a text dialog box.\n"
    "\tCPF_CrossLevelPassive =\t\t\t\t\t0x0000100000000000, // Property can point across levels, and will be serialized properly, but "
    "assumes it\'s target exists in-game (non-editor)\n"
    "\tCPF_CrossLevelActive =\t\t\t\t\t0x0000200000000000, // Property can point across levels, and will be serialized properly, and will "
    "be updated when the target is streamed in/out\n"
    "};\n"
    "\n"
    "// https://github.com/CodeRedModding/UnrealEngine3/blob/main/Development/Src/Core/Inc/UnObjBas.h#L316\n"
    "// Object Flags\n"
    "enum EObjectFlags : uint64_t\n"
    "{\n"
    "\tRF_NoFlags =\t\t\t\t\t\t\t0x000000000000000,\t// Object has no flags.\n"
    "\tRF_InSingularFunc =\t\t\t\t\t\t0x0000000000000002,\t// In a singular function.\n"
    "\tRF_StateChanged =\t\t\t\t\t\t0x0000000000000004,\t// Object did a state change.\n"
    "\tRF_DebugPostLoad =\t\t\t\t\t\t0x0000000000000008,\t// For debugging PostLoad calls.\n"
    "\tRF_DebugSerialize =\t\t\t\t\t\t0x0000000000000010,\t// For debugging Serialize calls.\n"
    "\tRF_DebugFinishDestroyed =\t\t\t\t0x0000000000000020,\t// For debugging FinishDestroy calls.\n"
    "\tRF_EdSelected =\t\t\t\t\t\t\t0x0000000000000040,\t// Object is selected in one of the editors browser windows.\n"
    "\tRF_ZombieComponent =\t\t\t\t\t0x0000000000000080,\t// This component\'s template was deleted, so should not be used.\n"
    "\tRF_Protected =\t\t\t\t\t\t\t0x0000000000000100, // Property is protected (may only be accessed from its owner class or "
    "subclasses).\n"
    "\tRF_ClassDefaultObject =\t\t\t\t\t0x0000000000000200,\t// this object is its class\'s default object.\n"
    "\tRF_ArchetypeObject =\t\t\t\t\t0x0000000000000400, // this object is a template for another object (treat like a class default "
    "object).\n"
    "\tRF_ForceTagExp =\t\t\t\t\t\t0x0000000000000800, // Forces this object to be put into the export table when saving a package "
    "regardless of outer.\n"
    "\tRF_TokenStreamAssembled =\t\t\t\t0x0000000000001000, // Set if reference token stream has already been assembled.\n"
    "\tRF_MisalignedObject =\t\t\t\t\t0x0000000000002000, // Object\'s size no longer matches the size of its C++ class (only used during "
    "make, for native classes whose properties have changed).\n"
    "\tRF_RootSet =\t\t\t\t\t\t\t0x0000000000004000, // Object will not be garbage collected, even if unreferenced.\n"
    "\tRF_BeginDestroyed =\t\t\t\t\t\t0x0000000000008000,\t// BeginDestroy has been called on the object.\n"
    "\tRF_FinishDestroyed =\t\t\t\t\t0x0000000000010000, // FinishDestroy has been called on the object.\n"
    "\tRF_DebugBeginDestroyed =\t\t\t\t0x0000000000020000, // Whether object is rooted as being part of the root set (garbage "
    "collection).\n"
    "\tRF_MarkedByCooker =\t\t\t\t\t\t0x0000000000040000,\t// Marked by content cooker.\n"
    "\tRF_LocalizedResource =\t\t\t\t\t0x0000000000080000, // Whether resource object is localized.\n"
    "\tRF_InitializedProps =\t\t\t\t\t0x0000000000100000, // whether InitProperties has been called on this object\n"
    "\tRF_PendingFieldPatches =\t\t\t\t0x0000000000200000, // @script patcher: indicates that this struct will receive additional member "
    "properties from the script patcher.\n"
    "\tRF_IsCrossLevelReferenced =\t\t\t\t0x0000000000400000,\t// This object has been pointed to by a cross-level reference, and "
    "therefore requires additional cleanup upon deletion.\n"
    "\tRF_Saved =\t\t\t\t\t\t\t\t0x0000000080000000, // Object has been saved via SavePackage (temporary).\n"
    "\tRF_Transactional =\t\t\t\t\t\t0x0000000100000000, // Object is transactional.\n"
    "\tRF_Unreachable =\t\t\t\t\t\t0x0000000200000000, // Object is not reachable on the object graph.\n"
    "\tRF_Public =\t\t\t\t\t\t\t\t0x0000000400000000, // Object is visible outside its package.\n"
    "\tRF_TagImp =\t\t\t\t\t\t\t\t0x0000000800000000,\t// Temporary import tag in load/save.\n"
    "\tRF_TagExp =\t\t\t\t\t\t\t\t0x0000001000000000,\t// Temporary export tag in load/save.\n"
    "\tRF_Obsolete =\t\t\t\t\t\t\t0x0000002000000000, // Object marked as obsolete and should be replaced.\n"
    "\tRF_TagGarbage =\t\t\t\t\t\t\t0x0000004000000000,\t// Check during garbage collection.\n"
    "\tRF_DisregardForGC =\t\t\t\t\t\t0x0000008000000000,\t// Object is being disregard for GC as its static and itself and all references "
    "are always loaded.\n"
    "\tRF_PerObjectLocalized =\t\t\t\t\t0x0000010000000000,\t// Object is localized by instance name, not by class.\n"
    "\tRF_NeedLoad =\t\t\t\t\t\t\t0x0000020000000000, // During load, indicates object needs loading.\n"
    "\tRF_AsyncLoading =\t\t\t\t\t\t0x0000040000000000, // Object is being asynchronously loaded.\n"
    "\tRF_NeedPostLoadSubobjects =\t\t\t\t0x0000080000000000, // During load, indicates that the object still needs to instance subobjects "
    "and fixup serialized component references.\n"
    "\tRF_Suppress =\t\t\t\t\t\t\t0x0000100000000000, // @warning: Mirrored in UnName.h. Suppressed log name.\n"
    "\tRF_InEndState =\t\t\t\t\t\t\t0x0000200000000000, // Within an EndState call.\n"
    "\tRF_Transient =\t\t\t\t\t\t\t0x0000400000000000, // Don\'t save object.\n"
    "\tRF_Cooked =\t\t\t\t\t\t\t\t0x0000800000000000, // Whether the object has already been cooked\n"
    "\tRF_LoadForClient =\t\t\t\t\t\t0x0001000000000000, // In-file load for client.\n"
    "\tRF_LoadForServer =\t\t\t\t\t\t0x0002000000000000, // In-file load for client.\n"
    "\tRF_LoadForEdit =\t\t\t\t\t\t0x0004000000000000, // In-file load for client.\n"
    "\tRF_Standalone =\t\t\t\t\t\t\t0x0008000000000000,\t// Keep object around for editing even if unreferenced.\n"
    "\tRF_NotForClient =\t\t\t\t\t\t0x0010000000000000, // Don\'t load this object for the game client.\n"
    "\tRF_NotForServer =\t\t\t\t\t\t0x0020000000000000, // Don\'t load this object for the game server.\n"
    "\tRF_NotForEdit =\t\t\t\t\t\t\t0x0040000000000000,\t// Don\'t load this object for the editor.\n"
    "\tRF_NeedPostLoad =\t\t\t\t\t\t0x0100000000000000, // Object needs to be postloaded.\n"
    "\tRF_HasStack =\t\t\t\t\t\t\t0x0200000000000000, // Has execution stack.\n"
    "\tRF_Native =\t\t\t\t\t\t\t\t0x0400000000000000, // Native (UClass only)\n"
    "\tRF_Marked =\t\t\t\t\t\t\t\t0x0800000000000000,\t// Marked (for debugging).\n"
    "\tRF_ErrorShutdown =\t\t\t\t\t\t0x1000000000000000, // ShutdownAfterError called.\n"
    "\tRF_PendingKill =\t\t\t\t\t\t0x2000000000000000, // Objects that are pending destruction (invalid for gameplay but valid objects).\n"
    "\tRF_MarkedByCookerTemp =\t\t\t\t\t0x4000000000000000,\t// Temporarily marked by content cooker (should be cleared).\n"
    "\tRF_CookedStartupObject =\t\t\t\t0x8000000000000000, // This object was cooked into a startup package.\n"
    "\n"
    "\tRF_ContextFlags =\t\t\t\t\t\t(RF_NotForClient | RF_NotForServer | RF_NotForEdit), // All context flags.\n"
    "\tRF_LoadContextFlags =\t\t\t\t\t(RF_LoadForClient | RF_LoadForServer | RF_LoadForEdit), // Flags affecting loading.\n"
    "\tRF_Load =\t\t\t\t\t\t\t\t(RF_ContextFlags | RF_LoadContextFlags | RF_Public | RF_Standalone | RF_Native | RF_Obsolete | "
    "RF_Protected | RF_Transactional | RF_HasStack | RF_PerObjectLocalized | RF_ClassDefaultObject | RF_ArchetypeObject | "
    "RF_LocalizedResource), // Flags to load from Unrealfiles.\n"
    "\tRF_Keep =\t\t\t\t\t\t\t\t(RF_Native | RF_Marked | RF_PerObjectLocalized | RF_MisalignedObject | RF_DisregardForGC | RF_RootSet | "
    "RF_LocalizedResource), // Flags to persist across loads.\n"
    "\tRF_ScriptMask =\t\t\t\t\t\t\t(RF_Transactional | RF_Public | RF_Transient | RF_NotForClient | RF_NotForServer | RF_NotForEdit | "
    "RF_Standalone), // Script-accessible flags.\n"
    "\tRF_UndoRedoMask =\t\t\t\t\t\t(RF_PendingKill), // Undo/ redo will store/ restore these\n"
    "\tRF_PropagateToSubObjects =\t\t\t\t(RF_Public | RF_ArchetypeObject | RF_Transactional), // Sub-objects will inherit these flags from "
    "their SuperObject.\n"
    "\n"
    "\t// custom flags\n"
    "\tRF_BadObjectFlags =\t\t\t\t\t\t(RF_BeginDestroyed | RF_FinishDestroyed | RF_PendingKill | RF_TagGarbage),\n"
    "\tRF_DefaultOrArchetypeFlags =\t\t\t(RF_ArchetypeObject | RF_ClassDefaultObject),\n"
    "\n"
    "\tRF_AllFlags =\t\t\t\t\t\t\t0xFFFFFFFFFFFFFFFF,\n"
    "};\n"
    "\n"
    "// https://github.com/CodeRedModding/UnrealEngine3/blob/main/Development/Src/Core/Inc/UnObjBas.h#L51\n"
    "// Package Flags\n"
    "enum EPackageFlags : uint32_t\n"
    "{\n"
    "\tPKG_AllowDownload =\t\t\t\t\t\t0x00000001,\t// Allow downloading package.\n"
    "\tPKG_ClientOptional =\t\t\t\t\t0x00000002,\t// Purely optional for clients.\n"
    "\tPKG_ServerSideOnly =\t\t\t\t\t0x00000004, // Only needed on the server side.\n"
    "\tPKG_Cooked =\t\t\t\t\t\t\t0x00000008,\t// Whether this package has been cooked for the target platform.\n"
    "\tPKG_Unsecure =\t\t\t\t\t\t\t0x00000010, // Not trusted.\n"
    "\tPKG_SavedWithNewerVersion =\t\t\t\t0x00000020,\t// Package was saved with newer version.\n"
    "\tPKG_Need =\t\t\t\t\t\t\t\t0x00008000,\t// Client needs to download this package.\n"
    "\tPKG_Compiling =\t\t\t\t\t\t\t0x00010000,\t// package is currently being compiled\n"
    "\tPKG_ContainsMap =\t\t\t\t\t\t0x00020000,\t// Set if the package contains a ULevel/ UWorld object\n"
    "\tPKG_Trash =\t\t\t\t\t\t\t\t0x00040000,\t// Set if the package was loaded from the trashcan\n"
    "\tPKG_DisallowLazyLoading =\t\t\t\t0x00080000,\t// Set if the archive serializing this package cannot use lazy loading\n"
    "\tPKG_PlayInEditor =\t\t\t\t\t\t0x00100000,\t// Set if the package was created for the purpose of PIE\n"
    "\tPKG_ContainsScript =\t\t\t\t\t0x00200000,\t// Package is allowed to contain UClasses and unrealscript\n"
    "\tPKG_ContainsDebugInfo =\t\t\t\t\t0x00400000,\t// Package contains debug info (for UDebugger)\n"
    "\tPKG_RequireImportsAlreadyLoaded =\t\t0x00800000,\t// Package requires all its imports to already have been loaded\n"
    "\tPKG_StoreCompressed =\t\t\t\t\t0x02000000,\t// Package is being stored compressed, requires archive support for compression\n"
    "\tPKG_StoreFullyCompressed =\t\t\t\t0x04000000,\t// Package is serialized normally, and then fully compressed after (must be "
    "decompressed before LoadPackage is called)\n"
    "\tPKG_ContainsFaceFXData =\t\t\t\t0x10000000,\t// Package contains FaceFX assets and/or animsets\n"
    "\tPKG_NoExportAllowed =\t\t\t\t\t0x20000000,\t// Package was NOT created by a modder.  Internal data not for export\n"
    "\tPKG_StrippedSource =\t\t\t\t\t0x40000000,\t// Source has been removed to compress the package size\n"
    "\tPKG_FilterEditorOnly =\t\t\t\t\t0x80000000,\t// Package has editor-only data filtered\n"
    "};\n"
    "\n"
    "// "
    "https://github.com/CodeRedModding/UnrealEngine3/blob/7bf53e29f620b0d4ca5c9bd063a2d2dbcee732fe/Development/Src/Core/Inc/"
    "UnObjBas.h#L98\n"
    "// Class Flags\n"
    "enum EClassFlags : uint32_t\n"
    "{\n"
    "\tCLASS_None =\t\t\t\t\t\t\t0x00000000, \n"
    "\tCLASS_Abstract =\t\t\t\t\t\t0x00000001, // Class is abstract and can\'t be instantiated directly.\n"
    "\tCLASS_Compiled =\t\t\t\t\t\t0x00000002, // Script has been compiled successfully.\n"
    "\tCLASS_Config =\t\t\t\t\t\t\t0x00000004, // Load object configuration at construction time.\n"
    "\tCLASS_Transient =\t\t\t\t\t\t0x00000008, // This object type can\'t be saved; null it out at save time.\n"
    "\tCLASS_Parsed =\t\t\t\t\t\t\t0x00000010, // Successfully parsed.\n"
    "\tCLASS_Localized =\t\t\t\t\t\t0x00000020, // Class contains localized text.\n"
    "\tCLASS_SafeReplace =\t\t\t\t\t\t0x00000040, // Objects of this class can be safely replaced with default or NULL.\n"
    "\tCLASS_Native =\t\t\t\t\t\t\t0x00000080, // Class is a native class - native interfaces will have CLASS_Native set, but not "
    "RF_Native.\n"
    "\tCLASS_NoExport =\t\t\t\t\t\t0x00000100, // Don\'t export to C++ header.\n"
    "\tCLASS_Placeable =\t\t\t\t\t\t0x00000200, // Allow users to create in the editor.\n"
    "\tCLASS_PerObjectConfig =\t\t\t\t\t0x00000400, // Handle object configuration on a per-object basis, rather than per-class.\n"
    "\tCLASS_NativeReplication =\t\t\t\t0x00000800, // Replication handled in C++.\n"
    "\tCLASS_EditInlineNew =\t\t\t\t\t0x00001000, // Class can be constructed from editinline New button..\n"
    "\tCLASS_CollapseCategories =\t\t\t\t0x00002000,\t// Display properties in the editor without using categories.\n"
    "\tCLASS_Interface =\t\t\t\t\t\t0x00004000, // Class is an interface.\n"
    "\tCLASS_HasInstancedProps =\t\t\t\t0x00200000, // class contains object properties which are marked \"instanced\" (or editinline "
    "export).\n"
    "\tCLASS_NeedsDefProps =\t\t\t\t\t0x00400000, // Class needs its defaultproperties imported.\n"
    "\tCLASS_HasComponents =\t\t\t\t\t0x00800000, // Class has component properties.\n"
    "\tCLASS_Hidden =\t\t\t\t\t\t\t0x01000000, // Don\'t show this class in the editor class browser or edit inline new menus.\n"
    "\tCLASS_Deprecated =\t\t\t\t\t\t0x02000000, // Don\'t save objects of this class when serializing.\n"
    "\tCLASS_HideDropDown =\t\t\t\t\t0x04000000, // Class not shown in editor drop down for class selection.\n"
    "\tCLASS_Exported =\t\t\t\t\t\t0x08000000, // Class has been exported to a header file.\n"
    "\tCLASS_Intrinsic =\t\t\t\t\t\t0x10000000, // Class has no unrealscript counter-part.\n"
    "\tCLASS_NativeOnly =\t\t\t\t\t\t0x20000000, // Properties in this class can only be accessed from native code.\n"
    "\tCLASS_PerObjectLocalized =\t\t\t\t0x40000000, // Handle object localization on a per-object basis, rather than per-class. \n"
    "\tCLASS_HasCrossLevelRefs =\t\t\t\t0x80000000, // This class has properties that are marked with CPF_CrossLevel \n"
    "\n"
    "\t// Deprecated, these values now match the values of the EClassCastFlags enum.\n"
    "\tCLASS_IsAUProperty =\t\t\t\t\t0x00008000,\n"
    "\tCLASS_IsAUObjectProperty =\t\t\t\t0x00010000,\n"
    "\tCLASS_IsAUBoolProperty =\t\t\t\t0x00020000,\n"
    "\tCLASS_IsAUState =\t\t\t\t\t\t0x00040000,\n"
    "\tCLASS_IsAUFunction =\t\t\t\t\t0x00080000,\n"
    "\tCLASS_IsAUStructProperty =\t\t\t\t0x00100000,\n"
    "\n"
    "\t// Flags to inherit from base class.\n"
    "\tCLASS_Inherit =\t\t\t\t\t\t\t(CLASS_Transient | CLASS_Config | CLASS_Localized | CLASS_SafeReplace | CLASS_PerObjectConfig | "
    "CLASS_PerObjectLocalized | CLASS_Placeable | CLASS_IsAUProperty | CLASS_IsAUObjectProperty | CLASS_IsAUBoolProperty | "
    "CLASS_IsAUStructProperty | CLASS_IsAUState | CLASS_IsAUFunction | CLASS_HasComponents | CLASS_Deprecated | CLASS_Intrinsic | "
    "CLASS_HasInstancedProps | CLASS_HasCrossLevelRefs),\n"
    "\n"
    "\t// These flags will be cleared by the compiler when the class is parsed during script compilation.\n"
    "\tCLASS_RecompilerClear =\t\t\t\t\t(CLASS_Inherit | CLASS_Abstract | CLASS_NoExport | CLASS_NativeReplication | CLASS_Native),\n"
    "\n"
    "\t// These flags will be inherited from the base class only for non-intrinsic classes.\n"
    "\tCLASS_ScriptInherit =\t\t\t\t\t(CLASS_Inherit | CLASS_EditInlineNew | CLASS_CollapseCategories),\n"
    "\n"
    "\tCLASS_AllFlags =\t\t\t\t\t\t0xFFFFFFFF,\n"
    "};\n"
    "\n"
    "// "
    "https://github.com/CodeRedModding/UnrealEngine3/blob/7bf53e29f620b0d4ca5c9bd063a2d2dbcee732fe/Development/Src/Core/Inc/"
    "UnObjBas.h#L195\n"
    "// Class Cast Flags\n"
    "enum EClassCastFlag : uint32_t\n"
    "{\n"
    "\tCASTCLASS_None =\t\t\t\t\t\t0x00000000,\n"
    "\tCASTCLASS_UField =\t\t\t\t\t\t0x00000001,\n"
    "\tCASTCLASS_UConst =\t\t\t\t\t\t0x00000002,\n"
    "\tCASTCLASS_UEnum =\t\t\t\t\t\t0x00000004,\n"
    "\tCASTCLASS_UStruct =\t\t\t\t\t\t0x00000008,\n"
    "\tCASTCLASS_UScriptStruct =\t\t\t\t0x00000010,\n"
    "\tCASTCLASS_UClass =\t\t\t\t\t\t0x00000020,\n"
    "\tCASTCLASS_UByteProperty =\t\t\t\t0x00000040,\n"
    "\tCASTCLASS_UIntProperty =\t\t\t\t0x00000080,\n"
    "\tCASTCLASS_UFloatProperty =\t\t\t\t0x00000100,\n"
    "\tCASTCLASS_UComponentProperty =\t\t\t0x00000200,\n"
    "\tCASTCLASS_UClassProperty =\t\t\t\t0x00000400,\n"
    "\tCASTCLASS_UInterfaceProperty =\t\t\t0x00001000,\n"
    "\tCASTCLASS_UNameProperty =\t\t\t\t0x00002000,\n"
    "\tCASTCLASS_UStrProperty =\t\t\t\t0x00004000,\n"
    "\n"
    "\t// These match the values of the old class flags to make conversion easier.\n"
    "\tCASTCLASS_UProperty =\t\t\t\t\t0x00008000,\n"
    "\tCASTCLASS_UObjectProperty =\t\t\t\t0x00010000,\n"
    "\tCASTCLASS_UBoolProperty =\t\t\t\t0x00020000,\n"
    "\tCASTCLASS_UState =\t\t\t\t\t\t0x00040000,\n"
    "\tCASTCLASS_UFunction =\t\t\t\t\t0x00080000,\n"
    "\tCASTCLASS_UStructProperty =\t\t\t\t0x00100000,\n"
    "\n"
    "\tCASTCLASS_UArrayProperty =\t\t\t\t0x00200000,\n"
    "\tCASTCLASS_UMapProperty =\t\t\t\t0x00400000,\n"
    "\tCASTCLASS_UDelegateProperty =\t\t\t0x00800000,\n"
    "\tCASTCLASS_UComponent =\t\t\t\t\t0x01000000,\n"
    "\n"
    "\tCASTCLASS_AllFlags =\t\t\t\t\t0xFFFFFFFF,\n"
    "};\n";

} // namespace PiecesOfCode

/*
# ========================================================================================= #
#
# ========================================================================================= #
*/
