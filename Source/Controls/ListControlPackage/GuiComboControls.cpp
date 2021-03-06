#include "GuiComboControls.h"

namespace vl
{
	namespace presentation
	{
		namespace controls
		{

/***********************************************************************
GuiComboBoxBase::CommandExecutor
***********************************************************************/

			GuiComboBoxBase::CommandExecutor::CommandExecutor(GuiComboBoxBase* _combo)
				:combo(_combo)
			{
			}

			GuiComboBoxBase::CommandExecutor::~CommandExecutor()
			{
			}

			void GuiComboBoxBase::CommandExecutor::SelectItem()
			{
				combo->SelectItem();
			}

/***********************************************************************
GuiComboBoxBase
***********************************************************************/

			bool GuiComboBoxBase::IsAltAvailable()
			{
				return false;
			}

			IGuiMenuService::Direction GuiComboBoxBase::GetSubMenuDirection()
			{
				return IGuiMenuService::Horizontal;
			}

			void GuiComboBoxBase::SelectItem()
			{
				styleController->OnItemSelected();
				ItemSelected.Execute(GetNotifyEventArguments());
			}

			void GuiComboBoxBase::OnBoundsChanged(compositions::GuiGraphicsComposition* sender, compositions::GuiEventArgs& arguments)
			{
				Size size=GetPreferredMenuClientSize();
				size.x=GetBoundsComposition()->GetBounds().Width();
				SetPreferredMenuClientSize(size);
			}

			GuiComboBoxBase::GuiComboBoxBase(IStyleController* _styleController)
				:GuiMenuButton(_styleController)
			{
				commandExecutor=new CommandExecutor(this);
				styleController=dynamic_cast<IStyleController*>(GetStyleController());
				styleController->SetCommandExecutor(commandExecutor.Obj());

				CreateSubMenu();
				SetCascadeAction(false);

				GetBoundsComposition()->BoundsChanged.AttachMethod(this, &GuiComboBoxBase::OnBoundsChanged);
			}

			GuiComboBoxBase::~GuiComboBoxBase()
			{
			}

/***********************************************************************
GuiComboBoxListControl
***********************************************************************/

			void GuiComboBoxListControl::RemoveStyleController()
			{
				if (itemStyleController)
				{
					SafeDeleteComposition(itemStyleController->GetBoundsComposition());
					itemStyleController = nullptr;
				}
			}

			void GuiComboBoxListControl::InstallStyleController(vint itemIndex)
			{
				if (itemBindingView != nullptr && itemStyleProvider)
				{
					if (itemIndex != -1)
					{
						auto item = itemBindingView->GetBindingValue(itemIndex);
						if (!item.IsNull())
						{
							itemStyleController = itemStyleProvider->CreateItemStyle(item);
							if (itemStyleController)
							{
								itemStyleController->SetText(GetText());
								itemStyleController->SetFont(GetFont());
								itemStyleController->SetVisuallyEnabled(GetVisuallyEnabled());

								auto composition = itemStyleController->GetBoundsComposition();
								composition->SetAlignmentToParent(Margin(0, 0, 0, 0));
								GetContainerComposition()->AddChild(composition);
							}
						}
					}
				}
			}

			void GuiComboBoxListControl::DisplaySelectedContent(vint itemIndex)
			{
				if(primaryTextView)
				{
					if(itemIndex==-1)
					{
						SetText(L"");
					}
					else if(primaryTextView->ContainsPrimaryText(itemIndex))
					{
						WString text=primaryTextView->GetPrimaryTextViewText(itemIndex);
						SetText(text);
						GetSubMenu()->Hide();
					}
				}

				RemoveStyleController();
				InstallStyleController(itemIndex);
			}

			void GuiComboBoxListControl::OnTextChanged(compositions::GuiGraphicsComposition* sender, compositions::GuiEventArgs& arguments)
			{
				if (itemStyleController)
				{
					itemStyleController->SetText(GetText());
				}
			}

			void GuiComboBoxListControl::OnFontChanged(compositions::GuiGraphicsComposition* sender, compositions::GuiEventArgs& arguments)
			{
				if (itemStyleController)
				{
					itemStyleController->SetFont(GetFont());
				}
				auto args = GetNotifyEventArguments();
				OnListControlAdoptedSizeInvalidated(nullptr, args);
			}

			void GuiComboBoxListControl::OnVisuallyEnabledChanged(compositions::GuiGraphicsComposition* sender, compositions::GuiEventArgs& arguments)
			{
				if (itemStyleController)
				{
					itemStyleController->SetVisuallyEnabled(GetVisuallyEnabled());
				}
			}

			void GuiComboBoxListControl::OnListControlAdoptedSizeInvalidated(compositions::GuiGraphicsComposition* sender, compositions::GuiEventArgs& arguments)
			{
				Size expectedSize(0, GetFont().size * 20);
				Size adoptedSize = containedListControl->GetAdoptedSize(expectedSize);

				Size clientSize = GetPreferredMenuClientSize();
				clientSize.y = adoptedSize.y + GetSubMenu()->GetClientSize().y - containedListControl->GetBoundsComposition()->GetBounds().Height();
				SetPreferredMenuClientSize(clientSize);

				if (GetSubMenuOpening())
				{
					GetSubMenu()->SetClientSize(clientSize);
				}
			}

			void GuiComboBoxListControl::OnListControlSelectionChanged(compositions::GuiGraphicsComposition* sender, compositions::GuiEventArgs& arguments)
			{
				DisplaySelectedContent(GetSelectedIndex());
				SelectItem();
				SelectedIndexChanged.Execute(GetNotifyEventArguments());
			}

			GuiComboBoxListControl::GuiComboBoxListControl(IStyleController* _styleController, GuiSelectableListControl* _containedListControl)
				:GuiComboBoxBase(_styleController)
				, styleController(_styleController)
				, containedListControl(_containedListControl)
			{
				styleController->SetTextVisible(true);
				TextChanged.AttachMethod(this, &GuiComboBoxListControl::OnTextChanged);
				FontChanged.AttachMethod(this, &GuiComboBoxListControl::OnFontChanged);
				VisuallyEnabledChanged.AttachMethod(this, &GuiComboBoxListControl::OnVisuallyEnabledChanged);

				containedListControl->SetMultiSelect(false);
				containedListControl->AdoptedSizeInvalidated.AttachMethod(this, &GuiComboBoxListControl::OnListControlAdoptedSizeInvalidated);
				containedListControl->SelectionChanged.AttachMethod(this, &GuiComboBoxListControl::OnListControlSelectionChanged);

				auto itemProvider = containedListControl->GetItemProvider();
				primaryTextView = dynamic_cast<GuiListControl::IItemPrimaryTextView*>(itemProvider->RequestView(GuiListControl::IItemPrimaryTextView::Identifier));
				itemBindingView = dynamic_cast<GuiListControl::IItemBindingView*>(itemProvider->RequestView(GuiListControl::IItemBindingView::Identifier));

				SelectedIndexChanged.SetAssociatedComposition(GetBoundsComposition());

				containedListControl->GetBoundsComposition()->SetAlignmentToParent(Margin(0, 0, 0, 0));
				GetSubMenu()->GetContainerComposition()->AddChild(containedListControl->GetBoundsComposition());
				SetFont(GetFont());
			}

			GuiComboBoxListControl::~GuiComboBoxListControl()
			{
				if(primaryTextView)
				{
					containedListControl->GetItemProvider()->ReleaseView(primaryTextView);
				}
			}

			GuiSelectableListControl* GuiComboBoxListControl::GetContainedListControl()
			{
				return containedListControl;
			}
			
			GuiComboBoxListControl::IItemStyleProvider* GuiComboBoxListControl::GetStyleProvider()
			{
				return itemStyleProvider.Obj();
			}

			Ptr<GuiComboBoxListControl::IItemStyleProvider> GuiComboBoxListControl::SetStyleProvider(Ptr<IItemStyleProvider> value)
			{
				RemoveStyleController();
				auto old = itemStyleProvider;
				if (itemStyleProvider)
				{
					itemStyleProvider->DetachComboBox();
				}

				itemStyleProvider = value;

				if (itemStyleProvider)
				{
					itemStyleProvider->AttachComboBox(this);
					styleController->SetTextVisible(false);
					InstallStyleController(GetSelectedIndex());
				}
				else
				{
					styleController->SetTextVisible(true);
				}

				StyleProviderChanged.Execute(GetNotifyEventArguments());
				return old;
			}

			vint GuiComboBoxListControl::GetSelectedIndex()
			{
				if(containedListControl->GetSelectedItems().Count()==1)
				{
					return containedListControl->GetSelectedItems()[0];
				}
				else
				{
					return -1;
				}
			}

			void GuiComboBoxListControl::SetSelectedIndex(vint value)
			{
				containedListControl->SetSelected(value, true);
			}

			description::Value GuiComboBoxListControl::GetSelectedItem()
			{
				auto selectedIndex = GetSelectedIndex();
				if (selectedIndex != -1)
				{
					if (itemBindingView)
					{
						return itemBindingView->GetBindingValue(selectedIndex);
					}
				}
				return description::Value();
			}

			GuiListControl::IItemProvider* GuiComboBoxListControl::GetItemProvider()
			{
				return containedListControl->GetItemProvider();
			}
		}
	}
}
