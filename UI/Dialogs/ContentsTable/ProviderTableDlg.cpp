// Displays the list of providers in a message service in a profile
#include <StdAfx.h>
#include <UI/Dialogs/ContentsTable/ProviderTableDlg.h>
#include <UI/Controls/ContentsTableListCtrl.h>
#include <MAPI/Cache/MapiObjects.h>
#include <MAPI/ColumnTags.h>
#include <UI/Dialogs/MFCUtilityFunctions.h>
#include <MAPI/MAPIProfileFunctions.h>
#include <UI/Dialogs/Editors/Editor.h>
#include <UI/Controls/SortList/ContentsData.h>
#include <MAPI/MAPIFunctions.h>

namespace dialog
{
	static std::wstring CLASS = L"CProviderTableDlg";

	CProviderTableDlg::CProviderTableDlg(
		_In_ ui::CParentWnd* pParentWnd,
		_In_ cache::CMapiObjects* lpMapiObjects,
		_In_ LPMAPITABLE lpMAPITable,
		_In_ LPPROVIDERADMIN lpProviderAdmin)
		: CContentsTableDlg(
			  pParentWnd,
			  lpMapiObjects,
			  IDS_PROVIDERS,
			  mfcmapiDO_NOT_CALL_CREATE_DIALOG,
			  nullptr,
			  lpMAPITable,
			  LPSPropTagArray(&columns::sptPROVIDERCols),
			  columns::PROVIDERColumns,
			  NULL,
			  MENU_CONTEXT_PROFILE_PROVIDERS)
	{
		TRACE_CONSTRUCTOR(CLASS);

		CContentsTableDlg::CreateDialogAndMenu(IDR_MENU_PROVIDER);

		m_lpProviderAdmin = lpProviderAdmin;
		if (m_lpProviderAdmin) m_lpProviderAdmin->AddRef();
	}

	CProviderTableDlg::~CProviderTableDlg()
	{
		TRACE_DESTRUCTOR(CLASS);
		// little hack to keep our releases in the right order - crash in o2k3 otherwise
		if (m_lpContentsTable) m_lpContentsTable->Release();
		m_lpContentsTable = nullptr;
		if (m_lpProviderAdmin) m_lpProviderAdmin->Release();
	}

	BEGIN_MESSAGE_MAP(CProviderTableDlg, CContentsTableDlg)
	ON_COMMAND(ID_OPENPROFILESECTION, OnOpenProfileSection)
	END_MESSAGE_MAP()

	_Check_return_ HRESULT CProviderTableDlg::OpenItemProp(
		int iSelectedItem,
		__mfcmapiModifyEnum /*bModify*/,
		_Deref_out_opt_ LPMAPIPROP* lppMAPIProp)
	{
		auto hRes = S_OK;

		output::DebugPrintEx(DBGOpenItemProp, CLASS, L"OpenItemProp", L"iSelectedItem = 0x%X\n", iSelectedItem);

		if (!lppMAPIProp || !m_lpContentsTableListCtrl || !m_lpProviderAdmin) return MAPI_E_INVALID_PARAMETER;

		*lppMAPIProp = nullptr;

		const auto lpListData = m_lpContentsTableListCtrl->GetSortListData(iSelectedItem);
		if (lpListData && lpListData->Contents())
		{
			const auto lpProviderUID = lpListData->Contents()->m_lpProviderUID;
			if (lpProviderUID)
			{
				hRes = EC_H(mapi::profile::OpenProfileSection(
					m_lpProviderAdmin, lpProviderUID, reinterpret_cast<LPPROFSECT*>(lppMAPIProp)));
			}
		}

		return hRes;
	}

	void CProviderTableDlg::OnOpenProfileSection()
	{
		if (!m_lpProviderAdmin) return;

		editor::CEditor MyUID(
			this, IDS_OPENPROFSECT, IDS_OPENPROFSECTPROMPT, CEDITOR_BUTTON_OK | CEDITOR_BUTTON_CANCEL);

		MyUID.InitPane(0, viewpane::DropDownPane::CreateGuid(IDS_MAPIUID, false));
		MyUID.InitPane(1, viewpane::CheckPane::Create(IDS_MAPIUIDBYTESWAPPED, false, false));

		if (!MyUID.DisplayDialog()) return;

		auto guid = MyUID.GetSelectedGUID(0, MyUID.GetCheck(1));
		SBinary MapiUID = {sizeof(GUID), reinterpret_cast<LPBYTE>(&guid)};

		LPPROFSECT lpProfSect = nullptr;
		EC_H_S(mapi::profile::OpenProfileSection(m_lpProviderAdmin, &MapiUID, &lpProfSect));
		if (lpProfSect)
		{
			auto lpTemp = mapi::safe_cast<LPMAPIPROP>(lpProfSect);
			if (lpTemp)
			{
				EC_H_S(DisplayObject(lpTemp, MAPI_PROFSECT, otContents, this));
				lpTemp->Release();
			}

			lpProfSect->Release();
		}

		MAPIFreeBuffer(MapiUID.lpb);
	}

	void CProviderTableDlg::HandleAddInMenuSingle(
		_In_ LPADDINMENUPARAMS lpParams,
		_In_ LPMAPIPROP lpMAPIProp,
		_In_ LPMAPICONTAINER /*lpContainer*/)
	{
		if (lpParams)
		{
			lpParams->lpProfSect = static_cast<LPPROFSECT>(lpMAPIProp);
		}

		addin::InvokeAddInMenu(lpParams);
	}
}