from collections import deque
grid = [
    ['A', 0,  0, '#', 0],
    [0, '#', 0, '#', 0],
    [0, 0,  0,  0, 0],
    ['#',0, '#', 'G',0],
    [0, 0,  0,  0, 0]
]
queue=deque()
r=0
c=0
start=(r,c)
visited=set()
queue.append(start)
visited.add(start)
directions=[
    (-1,0), # 上
    (1,0),  # 下
    (0,-1), # 左
    (0,1)   # 右
]
distance={}
distance[start]=0
parent={}
parent[start]=None
path=[]
while queue:
    r,c=queue.popleft()
    if grid[r][c]=='G':
        print('找到金币')
        print('距离：',distance[(r,c)])
        current=(r,c)
        while current is not None:
            path.append(current)
            current=parent[current]
        path.reverse()
        print('路径：',path)
        break
    for dr,dc in directions:
        nr=r+dr
        nc=c+dc
        #print('尝试：',nr,nc)
        if nr>=0 and nr<len(grid) and nc>=0 and nc<len(grid[0]) and grid[nr][nc]!='#' and (nr,nc) not in visited:
            queue.append((nr,nc))
            visited.add((nr,nc))
            distance[(nr,nc)]=distance[(r,c)]+1
            parent[(nr,nc)]=(r,c)
def get_action(current, next_position):

    r, c = current
    nr, nc = next_position

    if nr<r:
        return "UP"

    elif nr>r:
        return "DOWN"

    elif nc<c:
        return "LEFT"

    elif nc>c:
        return "RIGHT"
actions = []
for i in range(len(path)-1):

    current = path[i]

    next_position = path[i+1]

    action = get_action(current, next_position)

    actions.append(action)

print('操作:',actions)
