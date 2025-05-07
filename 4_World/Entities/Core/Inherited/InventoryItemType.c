class CarWheelType extends InventoryItemType
{
};

class ClothingType extends InventoryItemType
{
};

class ItemWatchType extends InventoryItemType
{
};

class ItemRadioType extends InventoryItemType
{
};

class ItemTransmitterType extends ClothingType
{
};

class ItemMegaphoneType extends InventoryItemType
{
};

class ItemCompassType extends InventoryItemType
{
};

class ItemGPSType extends InventoryItemType
{
};

class ItemMapType extends InventoryItemType
{
};

class ItemBookType extends InventoryItemType
{
};

class ItemOpticsType extends InventoryItemType
{
	/*
		Optics2D or in other words the model provided by `modelOptics` when `useModelOptics` is
		shared for all instances of a `ItemOpticsType`.
		Call the following methods during `ItemOptics.OnDrawOptics2D` to manipulate currently viewed optics.
	*/
	/*!
		\brief Set optics 2D model texture at provided selection index.
		\param index Selection index
		\param textureName Name of texture to set (/dz/.../*.paa)
	*/
	proto native void SetOptics2DTexture(int index, string textureName);
	/*!
		\brief Set optics 2D material at provided selection index.
		\param index Selection index
		\param materialName Name of material to set (/dz/.../*.rvmat)
	*/
	proto native void SetOptics2DMaterial(int index, string materialName);
	/*!
		\brief Finds selection index by name.
		\param selectionName Name of selection to find
		\param ignoreCase Whether search ignores case sensitivity
		\return Index of selection or -1 when none.
	*/
	proto native int FindOptics2DSelection(string selectionName, bool ignoreCase = true);
	/*!
		\brief Whether 2D model (`modelOptics`) is set and used for this type.
	*/
	proto native bool IsUsingOptics2DModel();
};

class ItemBarrelType extends InventoryItemType
{
};

class ItemSuppressorType extends InventoryItemType
{
};

class ItemGrenadeType extends InventoryItemType
{
};

class WeaponType extends InventoryItemType
{
};

class MagazineType extends InventoryItemType
{
};
