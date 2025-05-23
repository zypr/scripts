///@{ events

/**@enum	HandEventID
 * @brief	identifier for events mainly for rpc purposes
 **/
enum HandEventID
{
	UNKNOWN,
	TAKE,
	MOVETO,
	DROP,
	THROW,
	SWAP,
	FORCESWAP,
	DESTROY,
	CREATED,
	DESTROYED,
	REPLACE,
	REPLACE2,
	REPLACE3,
	REPLACED,
	HUMANCOMMAND_ACTION_FINISHED,
	HUMANCOMMAND_ACTION_ABORTED,
	ANIMEVENT_CHANGE_HIDE
};

enum JunctureRequestResult
{
	JUNCTURE_NOT_REQUIRED,
	JUNCTURE_ACQUIRED,
	JUNCTURE_DENIED,
	ERROR,
}

/**@class	HandEventBase
 * @brief	represents event that triggers transition from state to state
 **/
class HandEventBase
{
	int m_EventID = 0;
	int m_AnimationID = -1;
	DayZPlayer m_Player;
	ref InventoryLocation m_Src;

	//! For action guards. Although we can guarantee the item will exist for remotes and juncture, 
	//! we can't guarantee the conditions will be the same as when the action was first performed.
	//! So therefore we only perform maximal checks on the Client and then Server but then do minimal 
	//! checks on Juncture-Client and Juncture-Server and no checks on Remote-Client.
	bool m_IsJuncture;
	bool m_IsRemote;

	void HandEventBase (Man p = null, InventoryLocation src = null) { Class.CastTo(m_Player, p); m_Src = src; }
	HandEventID GetEventID () { return m_EventID; }

	bool IsAuthoritative()
	{
		return m_Player && (m_Player.GetInstanceType() != DayZPlayerInstanceType.INSTANCETYPE_CLIENT && m_Player.GetInstanceType() != DayZPlayerInstanceType.INSTANCETYPE_REMOTE);
	}

	bool IsOwner()
	{
		return m_Player && (m_Player.GetInstanceType() == DayZPlayerInstanceType.INSTANCETYPE_CLIENT || m_Player.GetInstanceType() == DayZPlayerInstanceType.INSTANCETYPE_AI_SERVER || m_Player.GetInstanceType() == DayZPlayerInstanceType.INSTANCETYPE_AI_SINGLEPLAYER);
	}

	bool IsProxy()
	{
		return m_Player && (m_Player.GetInstanceType() == DayZPlayerInstanceType.INSTANCETYPE_REMOTE || m_Player.GetInstanceType() == DayZPlayerInstanceType.INSTANCETYPE_AI_REMOTE);
	}

	void ReadFromContext (ParamsReadContext ctx) { } // actual read is in CreateHandEventFromContext

	void WriteToContext (ParamsWriteContext ctx)
	{
		ctx.Write(m_EventID);
		ctx.Write(m_Player);
		OptionalLocationWriteToContext(m_Src, ctx);
		ctx.Write(m_AnimationID);
	}

	InventoryLocation GetSrc () { return m_Src; }
	EntityAI GetSrcEntity ()
	{
		if (m_Src)
			return m_Src.GetItem();
		return null;
	}
	EntityAI GetSecondSrcEntity () { return null; }
	InventoryLocation GetDst () { return null; }
	int GetAnimationID () { return m_AnimationID; }
	bool AcquireInventoryJunctureFromServer (notnull Man player) { return false; }
	bool CheckRequest () { return true; }
	bool CheckRequestEx (InventoryValidation validation)
	{
		//! Do not check for action validity on remotes or when performing through juncture. 
		//! Juncture locks guarentee the item is safe to interact with and the server has validated the command at this point. 
		//! Checking at this point is both wasteful and can result in a failure which leads to desync
		if (validation.m_IsRemote || validation.m_IsJuncture)
			return true;
		
		return CheckRequest();
	}
	bool CanPerformEvent () { return true; }
	bool CanPerformEventEx (InventoryValidation validation) { return CanPerformEvent(); }
	bool CheckRequestSrc () { return true; }
	bool IsServerSideOnly () { return false; }

	static HandEventBase HandEventFactory (HandEventID id, Man p = null, InventoryLocation src = null)
	{
		switch (id)
		{
			case HandEventID.UNKNOWN: return null;
			case HandEventID.TAKE: return new HandEventTake(p, src);
			case HandEventID.MOVETO: return new HandEventMoveTo(p, src);
			case HandEventID.DROP: return new HandEventDrop(p, src);
			case HandEventID.THROW: return new HandEventThrow(p, src);
			case HandEventID.SWAP: return new HandEventSwap(p, src);
			case HandEventID.FORCESWAP: return new HandEventForceSwap(p, src);
			case HandEventID.DESTROY: return new HandEventDestroy(p, src);
			case HandEventID.CREATED: return new HandEventCreated(p, src);
			case HandEventID.DESTROYED: return new HandEventDestroyed(p, src);
			case HandEventID.REPLACE: return new HandEventDestroyAndReplaceWithNew(p, src);
			case HandEventID.REPLACE2: return new HandEventDestroyAndReplaceWithNewElsewhere(p, src);
			case HandEventID.REPLACE3: return new HandEventDestroyElsewhereAndReplaceWithNewInHands(p, src);
			case HandEventID.REPLACED: return new HandEventReplaced(p, src);
			case HandEventID.ANIMEVENT_CHANGE_HIDE: return HandAnimEventChanged(p, src);
			case HandEventID.HUMANCOMMAND_ACTION_FINISHED : return HandEventHumanCommandActionFinished(p, src);
			case HandEventID.HUMANCOMMAND_ACTION_ABORTED : return HandEventHumanCommandActionAborted(p, src);
		}
		Error("[hndfsm] HandEventFactory - unregistered hand event with id=" + id);
		return null;
	}
	
	static HandEventBase CreateHandEventFromContext (ParamsReadContext ctx)
	{
		int eventID = -1;
		if (!ctx.Read(eventID))
		{
			Error("[hndfsm] CreateHandEventFromContext - cannot read eventID");
			return null;
		}
		Man player;
		if (!ctx.Read(player))
		{
			Error("[hndfsm] CreateHandEventFromContext - cannot read player");
			return null;
		}		
		InventoryLocation src;
		OptionalLocationReadFromContext(src, ctx);

		int animID = -1;
		if (!ctx.Read(animID))
		{
			Error("[hndfsm] CreateHandEventFromContext - cannot read animID");
			return null;
		}
		HandEventBase b = HandEventFactory(eventID, player, src);
		if (b)
		{
			b.m_AnimationID = animID;
			b.ReadFromContext(ctx);
		}
		return b;
	}
	
	string DumpToString ()
	{
		string res = "{ HandEv id=" + typename.EnumToString(HandEventID, GetEventID());
		res = res + " pl=" + Object.GetDebugName(m_Player);
		res = res + " src=" + InventoryLocation.DumpToStringNullSafe(m_Src);
		res = res + " }";
		return res;
	}
	
	bool ReserveInventory()
	{
		InventoryLocation il = GetDst();
		if (!il)
		{
			il = GetSrc();
		}
			
		if (il && !m_Player.GetHumanInventory().HasInventoryReservation(il.GetItem(),il))
		{
			if (m_Player.GetHumanInventory().AddInventoryReservationEx(il.GetItem(), il, GameInventory.c_InventoryReservationTimeoutShortMS))
			{
				return true;
			}
		}

		return false;
	}
	
	void ClearInventoryReservation()
	{
		InventoryLocation il = GetDst();
		if (il)
		{
			il = GetSrc();
		}
		
		if (il)
		{
			m_Player.GetHumanInventory().ClearInventoryReservationEx(il.GetItem(), il);
		}
	}
};

class HandEventTake extends HandEventBase
{
	void HandEventTake (Man p = null, InventoryLocation src = null) { m_EventID = HandEventID.TAKE; }

	override InventoryLocation GetDst ()
	{
		InventoryLocation dst = new InventoryLocation;
		dst.SetHands(m_Player, GetSrcEntity());
		return dst;
	}

	override bool CheckRequestSrc ()
	{
		if (!GameInventory.CheckRequestSrc(m_Player, GetSrc(), GameInventory.c_MaxItemDistanceRadius))
		{
			Debug.InventoryHFSMLog("CANNOT perform", typename.EnumToString(HandEventID, GetEventID()) , "n/a", "CheckRequestSrc", m_Player.ToString() );
			if (LogManager.IsSyncLogEnable()) syncDebugPrint("[cheat] HandleInputData man=" + Object.GetDebugName(m_Player) + " failed src1 check with cmd=" + typename.EnumToString(HandEventID, GetEventID()) + " src1=" + InventoryLocation.DumpToStringNullSafe(GetSrc()));
			return false; // stale packet
		}
		return true;
	}
	
	override bool CheckRequest ()
	{
		return GameInventory.CheckMoveToDstRequest(m_Player, GetSrc(), GetDst(), GameInventory.c_MaxItemDistanceRadius);
	}
	
	override bool CanPerformEventEx(InventoryValidation validation)
	{
		if (validation.m_IsJuncture)
		{
			return true;
		}
		
		if (!GameInventory.LocationCanMoveEntity(GetSrc(), GetDst()))
		{
			#ifdef ENABLE_LOGGING
			if ( LogManager.IsInventoryHFSMLogEnable() )
			{	
				Debug.InventoryHFSMLog("CANNOT perform", typename.EnumToString(HandEventID, GetEventID()) , "n/a", "CanPerformEvent", m_Player.ToString() );
			}
			#endif
			//if (LogManager.IsInventoryHFSMLogEnable()) hndDebugPrint("[desync] HandleInputData man=" + Object.GetDebugName(m_Player) + " CANNOT perform ev=" + DumpToString());
			return false;
		}
		
		return super.CanPerformEventEx(validation);
	}
	
	override bool AcquireInventoryJunctureFromServer (notnull Man player)
	{
		InventoryLocation src = GetSrc();
		InventoryLocation dst = GetDst();
		if (src && dst)
		{
			return TryAcquireInventoryJunctureFromServer(player, src, dst);
		}
		Error("[hndfsm] HandEventTake. AcquireInventoryJunctureFromServer: no src or dst for ev=" + DumpToString());
		return JunctureRequestResult.ERROR;
	}
};

class HandEventMoveTo extends HandEventBase
{
	ref InventoryLocation m_Dst; /// destination for item in hands

	void HandEventMoveTo (Man p = null, InventoryLocation src = null, InventoryLocation dst = null) { m_EventID = HandEventID.MOVETO; m_Dst = dst; }

	override void ReadFromContext (ParamsReadContext ctx)
	{
		m_Dst = new InventoryLocation;
		super.ReadFromContext(ctx);
		m_Dst.ReadFromContext(ctx);
	}
	override void WriteToContext (ParamsWriteContext ctx)
	{
		super.WriteToContext(ctx);
		m_Dst.WriteToContext(ctx);
	}

	override InventoryLocation GetDst () { return m_Dst; }
	
	override bool CheckRequestSrc ()
	{
		if (!GameInventory.CheckRequestSrc(m_Player, GetSrc(), GameInventory.c_MaxItemDistanceRadius))
		{
			if (LogManager.IsSyncLogEnable()) syncDebugPrint("[cheat] HandleInputData man=" + Object.GetDebugName(m_Player) + " failed src1 check with cmd=" + typename.EnumToString(HandEventID, GetEventID()) + " src1=" + InventoryLocation.DumpToStringNullSafe(GetSrc()));
			return false; // stale packet
		}
		return true;
	}
	
	override bool CheckRequest ()
	{
		return GameInventory.CheckMoveToDstRequest(m_Player, GetSrc(), GetDst(), GameInventory.c_MaxItemDistanceRadius);
	}
	
	override bool CanPerformEvent ()
	{
		if (!GameInventory.LocationCanMoveEntity(GetSrc(), GetDst()))
		{
			#ifdef ENABLE_LOGGING
			if ( LogManager.IsInventoryHFSMLogEnable() )
			{	
				Debug.InventoryHFSMLog("CANNOT perform", typename.EnumToString(HandEventID, GetEventID()) , "n/a", "CanPerformEvent", m_Player.ToString() );
			}
			#endif
			return false;
		}
		return true;
	}

	override bool AcquireInventoryJunctureFromServer (notnull Man player)
	{
		InventoryLocation src = GetSrc();
		InventoryLocation dst = GetDst();
		if (src && dst)
		{
			return TryAcquireInventoryJunctureFromServer(player, src, dst);
		}
		Error("[hndfsm] HandEventMoveTo. AcquireInventoryJunctureFromServer: no src or dst for ev=" + DumpToString());
		return JunctureRequestResult.ERROR;
	}

	override string DumpToString ()
	{
		string res = "{ HandEventMoveTo id=" + typename.EnumToString(HandEventID, GetEventID()) + " pl=" + Object.GetDebugName(m_Player) + " src=" + InventoryLocation.DumpToStringNullSafe(m_Src)  + " dst=" + InventoryLocation.DumpToStringNullSafe(m_Dst) + " }";
		return res;
	}
};

//! Abstracted event, not to be used, only inherited
class HandEventRemove extends HandEventBase
{
	override InventoryLocation GetDst ()
	{
		return m_Dst;
	}

	override bool CheckRequestSrc ()
	{
		if (!GameInventory.CheckRequestSrc(m_Player, GetSrc(), GameInventory.c_MaxItemDistanceRadius))
		{
			#ifdef ENABLE_LOGGING
			if ( LogManager.IsInventoryHFSMLogEnable() )
			{	
				Debug.InventoryHFSMLog("Check src - failed, src = " + InventoryLocation.DumpToStringNullSafe(GetSrc()), typename.EnumToString(HandEventID, GetEventID()) , "n/a", "CheckRequestSrc", m_Player.ToString() );
			}
			#endif
			return false; // stale packet
		}
		return true;
	}
	
	override bool CheckRequest ()
	{
		return GameInventory.CheckMoveToDstRequest(m_Player, GetSrc(), GetDst(), GameInventory.c_MaxItemDistanceRadius);
	}
	
	override bool CanPerformEvent ()
	{
		InventoryLocation src = GetSrc();
		InventoryLocation dst = GetDst();

		if (!dst)
		{
			#ifdef ENABLE_LOGGING
			if ( LogManager.IsInventoryHFSMLogEnable() )
			{
				Debug.InventoryHFSMLog("CANNOT perform. Dst location is NULL!", typename.EnumToString(HandEventID, GetEventID()) , "n/a", "CanPerformEvent", m_Player.ToString() );
			}
			#endif
			return false;
		}
		
		if (dst.GetType() == InventoryLocationType.UNKNOWN)
		{
			#ifdef ENABLE_LOGGING
			if ( LogManager.IsInventoryHFSMLogEnable() )
			{
				Debug.InventoryHFSMLog("CANNOT perform. Dst location type is UNKNOWN!", typename.EnumToString(HandEventID, GetEventID()) , "n/a", "CanPerformEvent", m_Player.ToString() );
			}
			#endif
			return false;
		}
		
		if (!GameInventory.LocationCanMoveEntity(src, dst))
		{
			#ifdef ENABLE_LOGGING
			if ( LogManager.IsInventoryHFSMLogEnable() )
			{
				Debug.InventoryHFSMLog("CANNOT perform", typename.EnumToString(HandEventID, GetEventID()) , "n/a", "CanPerformEvent", m_Player.ToString() );
			}
			#endif
			return false;
		}
		return true;
	}

	override bool AcquireInventoryJunctureFromServer (notnull Man player)
	{
		InventoryLocation src = GetSrc();
		InventoryLocation dst = GetDst();
		if (src && dst)
		{
			return TryAcquireInventoryJunctureFromServer(player, src, dst);
		}
		Error("[hndfsm] HandEventThrow. AcquireInventoryJunctureFromServer: no src or dst for ev=" + DumpToString());
		return JunctureRequestResult.ERROR;
	}
	
	ref InventoryLocation m_Dst;
};

class HandEventDrop extends HandEventRemove
{
	void HandEventDrop (Man p = null, InventoryLocation src = null)
	{
		m_EventID = HandEventID.DROP;
		m_CanPerformDrop = true;
	}

	override void ReadFromContext(ParamsReadContext ctx)
	{
		super.ReadFromContext(ctx);

		if (!m_Dst)
		{
			m_Dst = new InventoryLocation();
		}
		
		ctx.Read(m_CanPerformDrop);
		OptionalLocationReadFromContext(m_Dst, ctx);
	}

	override void WriteToContext(ParamsWriteContext ctx)
	{
		super.WriteToContext(ctx);

		ctx.Write(m_CanPerformDrop);
		OptionalLocationWriteToContext(m_Dst, ctx);
	}

	override bool CheckRequestEx(InventoryValidation validation)
	{
		//! Check to see if this is the initial call from the server (but the event originated from a client)
		if (!validation.m_IsJuncture && IsAuthoritative())
		{
			m_CanPerformDrop = GameInventory.SetGroundPosByOwner(m_Player, GetSrcEntity(), m_Dst);
		}

		if (!m_CanPerformDrop)
		{
			validation.m_Reason = InventoryValidationReason.DROP_PREVENTED;
			return false;
		}

		return super.CheckRequestEx(validation);
	}
	
	override bool CanPerformEventEx(InventoryValidation validation)
	{
		if (!m_CanPerformDrop)
		{
			return false;
		}

		//! On multiplayer client, if this is the initial call then we are waiting for the server to setup this event still
		if (!validation.m_IsJuncture && !validation.m_IsRemote && !GetDst() && (GetGame().IsMultiplayer() && GetGame().IsClient()))
		{
			return true;
		}
		
		//! Singleplayer or server was initial caller
		if (!validation.m_IsRemote && !GetDst())
		{
			m_Dst = new InventoryLocation();
			m_CanPerformDrop = GameInventory.SetGroundPosByOwner(m_Player, GetSrcEntity(), m_Dst);
			
			if (!m_CanPerformDrop)
			{
				validation.m_Reason = InventoryValidationReason.DROP_PREVENTED;
				return false;
			}
		}

		return super.CanPerformEventEx(validation);
	}
	
	bool m_CanPerformDrop;
};

class HandEventThrow extends HandEventRemove
{
	void HandEventThrow (Man p = null, InventoryLocation src = null) 
	{
		m_EventID = HandEventID.THROW; 
		if( src )
		{
			vector mat[4];
			EntityAI entity = GetSrcEntity();
			if (entity)
			{
				m_Dst = new InventoryLocation;
				entity.GetTransform(mat);
				m_Dst.SetGround(entity, mat);
			}
			else
			{
				Error("[hndfsm] HandEventThrow src entity null!");
			}
		}
	}
	
	override void ReadFromContext (ParamsReadContext ctx)
	{
		super.ReadFromContext(ctx);
		
		m_Dst = new InventoryLocation;
		m_Dst.ReadFromContext(ctx);

		float x, y, z;
		ctx.Read(x);
		ctx.Read(y);
		ctx.Read(z);
		m_Force[0] = x;
		m_Force[1] = y;
		m_Force[2] = z;
	}
	
	override void WriteToContext (ParamsWriteContext ctx)
	{
		super.WriteToContext(ctx);
		
		m_Dst.WriteToContext(ctx);

		ctx.Write(m_Force[0]);
		ctx.Write(m_Force[1]);
		ctx.Write(m_Force[2]);
	}

	void SetForce(vector force)	{ m_Force = force; }
	vector GetForce() { return m_Force; }
	
	vector m_Force;	
};

class HandEventSwap extends HandEventBase
{
	// super.m_Src == src location of new item (that will be taken)
	ref InventoryLocation m_Dst; /// destination for new item (i.e. hands)
	ref InventoryLocation m_Src2; /// src of old item in hands
	ref InventoryLocation m_Dst2; /// destination for old item that was in hands
	int m_Animation2ID = -1;

	void HandEventSwap (Man p = null, InventoryLocation src = null, InventoryLocation src2 = null, InventoryLocation dst = null, InventoryLocation dst2 = null)
	{
		m_EventID = HandEventID.SWAP; 
		m_Dst = dst;
		m_Src2 = src2;
		m_Dst2 = dst2;
	}
	
	override void ReadFromContext (ParamsReadContext ctx)
	{
		super.ReadFromContext(ctx);

		m_Src2 = new InventoryLocation;
		m_Dst = new InventoryLocation;
		m_Dst2 = new InventoryLocation;

		m_Src2.ReadFromContext(ctx);
		m_Dst.ReadFromContext(ctx);
		m_Dst2.ReadFromContext(ctx);
		ctx.Read(m_Animation2ID);
	}

	override void WriteToContext (ParamsWriteContext ctx)
	{
		super.WriteToContext(ctx);

		m_Src2.WriteToContext(ctx);
		m_Dst.WriteToContext(ctx);
		m_Dst2.WriteToContext(ctx);
		ctx.Write(m_Animation2ID);
	}

	override InventoryLocation GetDst ()
	{
		return m_Dst;
	}
	
	override EntityAI GetSecondSrcEntity()
	{
		return m_Src2.GetItem();
	}
	
	override bool CheckRequestSrc ()
	{
		//return false;

		if (!GameInventory.CheckRequestSrc(m_Player, GetSrc(), GameInventory.c_MaxItemDistanceRadius))
		{
			if (LogManager.IsSyncLogEnable()) syncDebugPrint("[cheat] HandleInputData man=" + Object.GetDebugName(m_Player) + " failed src1 check with cmd=" + typename.EnumToString(HandEventID, GetEventID()) + " src1=" + InventoryLocation.DumpToStringNullSafe(GetSrc()));
			return false; // stale packet
		}
		if (!GameInventory.CheckRequestSrc(m_Player, m_Src2, GameInventory.c_MaxItemDistanceRadius))
		{
			if (LogManager.IsSyncLogEnable()) syncDebugPrint("[cheat] HandleInputData man=" + Object.GetDebugName(m_Player) + " failed src2 check with cmd=" + typename.EnumToString(HandEventID, GetEventID()) + " src2=" + InventoryLocation.DumpToStringNullSafe(m_Src2));
			return false; // stale packet
		}
		return true;
	}
				
	override bool CheckRequest ()
	{
		if (!GameInventory.CheckSwapItemsRequest(m_Player, m_Src, m_Src2, m_Dst, m_Dst2, GameInventory.c_MaxItemDistanceRadius))
		{	
			#ifdef ENABLE_LOGGING
			if ( LogManager.IsInventoryHFSMLogEnable() )
			{	
				Debug.InventoryHFSMLog("CheckSwapItemsRequest - failed", typename.EnumToString(HandEventID, GetEventID()) , "n/a", "CheckRequest", m_Player.ToString() );
			}
			#endif
		}
  		else if (!m_Player.GetHumanInventory().CanAddSwappedEntity(m_Src, m_Src2, m_Dst, m_Dst2))
		{	
			#ifdef ENABLE_LOGGING
			if ( LogManager.IsInventoryHFSMLogEnable() )
			{	
				Debug.InventoryHFSMLog("CanAddSwappedEntity - failed", typename.EnumToString(HandEventID, GetEventID()) , "n/a", "CheckRequest", m_Player.ToString() );
			}
			#endif
		}
		else
			return true;

		return false;
	}
	
	override bool CanPerformEvent ()
	{
		if (GameInventory.CanForceSwapEntitiesEx(GetSrc().GetItem(), m_Dst, m_Src2.GetItem(), m_Dst2))
			return true;
		
		#ifdef ENABLE_LOGGING
		if ( LogManager.IsInventoryHFSMLogEnable() )
		{	
			Debug.InventoryHFSMLog("CANNOT perform", typename.EnumToString(HandEventID, GetEventID()) , "n/a", "CanPerformEvent", m_Player.ToString() );
		}
		#endif
		return false;
	}

	override bool AcquireInventoryJunctureFromServer (notnull Man player)
	{
		return TryAcquireTwoInventoryJuncturesFromServer(player, m_Src, m_Src2, m_Dst, m_Dst2);
	}

	override string DumpToString ()
	{
		return "{ HandEventSwap id=" + typename.EnumToString(HandEventID, GetEventID()) + " pl=" + Object.GetDebugName(m_Player) + " src1=" + InventoryLocation.DumpToStringNullSafe(m_Src) + " src2=" + InventoryLocation.DumpToStringNullSafe(m_Src2) + " dst1=" + InventoryLocation.DumpToStringNullSafe(m_Dst) + " dst2=" + InventoryLocation.DumpToStringNullSafe(m_Dst2) + " }";
	}
	
	void CheckAndExecuteForceStandUp()
	{
		DayZPlayer player = DayZPlayer.Cast(m_Player);
		player.ForceStandUpForHeavyItemsSwap( m_Src.GetItem(), m_Src2.GetItem() );
	}
	
	override bool ReserveInventory()
	{
		if (!m_Player.GetHumanInventory().HasInventoryReservation(m_Dst.GetItem(), m_Dst) && !m_Player.GetHumanInventory().HasInventoryReservation(m_Dst2.GetItem(), m_Dst2))
		{
			if (m_Player.GetHumanInventory().AddInventoryReservationEx(m_Dst.GetItem(), m_Dst, GameInventory.c_InventoryReservationTimeoutShortMS))
			{
				if (m_Player.GetHumanInventory().AddInventoryReservationEx(m_Dst2.GetItem(), m_Dst2, GameInventory.c_InventoryReservationTimeoutShortMS))
				{
					return true;
				}
				else
				{
					m_Player.GetHumanInventory().ClearInventoryReservationEx(m_Dst.GetItem(), m_Dst);
				}
			}
		}
		return false;
	}
	
	override void ClearInventoryReservation()
	{
		m_Player.GetHumanInventory().ClearInventoryReservationEx(m_Dst.GetItem(), m_Dst);
		m_Player.GetHumanInventory().ClearInventoryReservationEx(m_Dst2.GetItem(), m_Dst2);
	}
};

class HandEventForceSwap extends HandEventSwap
{	
	void HandEventForceSwap (Man p = null, InventoryLocation src = null, InventoryLocation src2 = null, InventoryLocation dst = null, InventoryLocation dst2 = null) { m_EventID = HandEventID.FORCESWAP; }
	
	override bool CheckRequest ()
	{
		bool test1 = false;
		EntityAI inHands = m_Player.GetHumanInventory().GetEntityInHands();
		if (GetSrcEntity() && inHands && m_Dst && m_Dst.IsValid())
		{
			test1 = GameInventory.CheckSwapItemsRequest(m_Player, m_Src, m_Src2, m_Dst, m_Dst2, GameInventory.c_MaxItemDistanceRadius);
			#ifdef ENABLE_LOGGING
			if ( LogManager.IsInventoryHFSMLogEnable() )
			{
				if (!test1)	
					Debug.InventoryHFSMLog("CheckSwapItemsRequest failed", typename.EnumToString(HandEventID, GetEventID()) , "n/a", "CheckRequest", m_Player.ToString() );
	 		}
			#endif
		}
		return test1;
	}
	
	override bool CanPerformEvent ()
	{
		bool test2 = GameInventory.CanForceSwapEntitiesEx(m_Src.GetItem(), m_Dst, m_Src2.GetItem(), m_Dst2); // null here means 'do not search for dst2' (already have valid one from constructor)
		#ifdef ENABLE_LOGGING
		if ( LogManager.IsInventoryHFSMLogEnable() )
		{
			if (!test2)	
				Debug.InventoryHFSMLog("CanForceSwapEntitiesEx failed", typename.EnumToString(HandEventID, GetEventID()) , "n/a", "CanPerformEvent", m_Player.ToString() );
		}
		#endif
		return test2;
	}
};

class HandEventDestroy extends HandEventBase
{
	void HandEventDestroy (Man p = null, InventoryLocation src = null) { m_EventID = HandEventID.DESTROY; }
	
	override bool IsServerSideOnly () { return true; }
};

class HandEventCreated extends HandEventBase
{
	void HandEventCreated (Man p = null, InventoryLocation src = null) { m_EventID = HandEventID.CREATED; }
};

class HandEventDestroyed extends HandEventBase
{
	void HandEventDestroyed (Man p = null, InventoryLocation src = null) { m_EventID = HandEventID.DESTROYED; }
};

class HandEventReplaceWithNewBase extends HandEventBase
{
	string m_Type;
	ref ReplaceItemWithNewLambdaBase m_Lambda;

	void HandEvengReplaceWithNewBase (Man p = null, InventoryLocation src = null, ReplaceItemWithNewLambdaBase lambda = NULL) { m_EventID = HandEventID.REPLACE; m_Lambda = lambda; }

	override void ReadFromContext (ParamsReadContext ctx)
	{
		super.ReadFromContext(ctx);
		ctx.Read(m_Type);
		Error("[hndfsm] HandEventDestroyAndReplaceWithNew - Cannot serialize lambda (read)");
	}
	override void WriteToContext (ParamsWriteContext ctx)
	{
		super.WriteToContext(ctx);
		ctx.Write(m_Type);
		Error("[hndfsm] HandEventDestroyAndReplaceWithNew - Cannot serialize lambda (write)");
	}

	override bool IsServerSideOnly () { return true; }
	
	override InventoryLocation GetDst ()
	{
		InventoryLocation dst = new InventoryLocation;
		dst.SetHands(m_Player, GetSrcEntity());
		return dst;
	}
	
	override string DumpToString ()
	{
		string res = "{ HandEvenReplaceWithNewBase id=" + typename.EnumToString(HandEventID, GetEventID()) + " pl=" + Object.GetDebugName(m_Player) + " src=" + InventoryLocation.DumpToStringNullSafe(GetSrc()) + " lambda=" + m_Lambda + " dst=" + InventoryLocation.DumpToStringNullSafe(GetDst()) + " }";
		return res;
	}
};

class HandEventDestroyAndReplaceWithNew extends HandEventReplaceWithNewBase
{
	void HandEventDestroyAndReplaceWithNew (Man p = null, InventoryLocation src = null, ReplaceItemWithNewLambdaBase lambda = NULL) { m_EventID = HandEventID.REPLACE; m_Lambda = lambda; }
};

class HandEventDestroyAndReplaceWithNewElsewhere extends HandEventDestroyAndReplaceWithNew
{
	void HandEventDestroyAndReplaceWithNewElsewhere (Man p = null, InventoryLocation src = null, ReplaceItemWithNewLambdaBase lambda = NULL) { m_EventID = HandEventID.REPLACE3; m_Lambda = lambda; }
};

class HandEventDestroyElsewhereAndReplaceWithNewInHands extends HandEventReplaceWithNewBase
{
	void HandEventDestroyElsewhereAndReplaceWithNewInHands (Man p = null, InventoryLocation src = null, ReplaceItemWithNewLambdaBase lambda = NULL) { m_EventID = HandEventID.REPLACE3; m_Lambda = lambda; }
};

class HandEventReplaced extends HandEventBase
{
	void HandEventReplaced (Man p = null, InventoryLocation src = null) { m_EventID = HandEventID.REPLACED; }
};

// anim events

class HandAnimEventChanged extends HandEventBase
{
	void HandAnimEventChanged (Man p = null, InventoryLocation src = null) { m_EventID = HandEventID.ANIMEVENT_CHANGE_HIDE; }
};

HandEventBase HandAnimEventFactory (WeaponEvents type, Man p = null, InventoryLocation src = null)
{
	switch (type)
	{
		case WeaponEvents.CHANGE_HIDE: return new HandAnimEventChanged(p, src);
		case WeaponEvents.CHANGE_SHOW: return new HandAnimEventChanged(p, src);
	}
	return null;
}

/**@brief		triggered when animation action finishes
 **/
class HandEventHumanCommandActionFinished extends HandEventBase
{
	void HandEventHumanCommandActionFinished (Man p = null, InventoryLocation src = null) { m_EventID = HandEventID.HUMANCOMMAND_ACTION_FINISHED; }
};
/**@brief		triggered when animation action aborts
 **/
class HandEventHumanCommandActionAborted extends HandEventBase
{
	void HandEventHumanCommandActionAborted (Man p = null, InventoryLocation src = null) { m_EventID = HandEventID.HUMANCOMMAND_ACTION_ABORTED; }
};

///@} events

